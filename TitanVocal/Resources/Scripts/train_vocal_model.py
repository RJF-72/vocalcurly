# C:/Vocal Plugin/TitanVocal/Resources/Scripts/train_vocal_model.py
import torch
import torch.nn as nn
import numpy as np
import librosa
from pathlib import Path
from typing import List, Tuple, Dict
import argparse
import os


class PositionalEncoding(nn.Module):
    def __init__(self, d_model, dropout=0.1, max_len=5000):
        super().__init__()
        self.dropout = nn.Dropout(p=dropout)

        position = torch.arange(max_len).unsqueeze(1)
        div_term = torch.exp(torch.arange(0, d_model, 2) * (-np.log(10000.0) / d_model))

        pe = torch.zeros(max_len, d_model)
        pe[:, 0::2] = torch.sin(position * div_term)
        pe[:, 1::2] = torch.cos(position * div_term)

        self.register_buffer('pe', pe)

    def forward(self, x):
        # x: (batch, seq_len, d_model)
        x = x + self.pe[:x.size(1)]
        return self.dropout(x)


class MultiTaskHead(nn.Module):
    def __init__(self, d_model, hidden_size, num_layers):
        super().__init__()
        layers = []
        current_size = d_model
        for i in range(num_layers):
            out_size = hidden_size if i == num_layers - 1 else d_model
            layers.append(nn.Linear(current_size, out_size))
            if i < num_layers - 1:
                layers.append(nn.GELU())
                layers.append(nn.LayerNorm(d_model))
            current_size = out_size
        self.network = nn.Sequential(*layers)

    def forward(self, x):
        return self.network(x)


class VocalRepairTransformer(nn.Module):
    """Advanced transformer-based vocal repair model (complete)"""

    def __init__(self, d_model=512, nhead=8, num_layers=6, dropout=0.1):
        super().__init__()
        self.d_model = d_model

        # Spectral encoder (from 1024-bin spectrum to model dim)
        self.spectral_encoder = nn.Sequential(
            nn.Linear(1024, d_model),
            nn.ReLU(),
            nn.LayerNorm(d_model),
            nn.Dropout(dropout)
        )

        # Positional encoding
        self.pos_encoder = PositionalEncoding(d_model, dropout)

        # Transformer encoder
        encoder_layers = nn.TransformerEncoderLayer(
            d_model=d_model, nhead=nhead, dropout=dropout,
            dim_feedforward=d_model * 4, activation='gelu'
        )
        self.transformer = nn.TransformerEncoder(encoder_layers, num_layers)

        # Multi-task heads produce heterogeneous feature sizes
        self.pitch_decoder = MultiTaskHead(d_model, 256, 3)
        self.formant_decoder = MultiTaskHead(d_model, 512, 3)
        self.noise_decoder = MultiTaskHead(d_model, 128, 2)
        self.breath_decoder = MultiTaskHead(d_model, 64, 2)

        # Fusion to common feature space for reconstruction
        self.fusion = nn.Sequential(
            nn.Linear(256 + 512 + 128 + 64, d_model),
            nn.GELU(),
            nn.LayerNorm(d_model)
        )

        # Spectral decoder back to 1024-bin spectrum
        self.spectral_decoder = nn.Sequential(
            nn.Linear(d_model, d_model * 2),
            nn.GELU(),
            nn.Linear(d_model * 2, 1024),
            nn.Tanh()
        )

    def forward(self, x: torch.Tensor, task_weights: torch.Tensor = None):
        # x: (batch, seq_len, 1024)
        batch_size, seq_len, _ = x.shape

        # Encode spectral features
        x = self.spectral_encoder(x)  # (batch, seq_len, d_model)

        # Add positional encoding and transpose for transformer
        x = self.pos_encoder(x)
        x = x.transpose(0, 1)  # (seq_len, batch, d_model)

        # Transformer processing
        x = self.transformer(x)
        x = x.transpose(0, 1)  # (batch, seq_len, d_model)

        # Multi-task decoding
        pitch_out = self.pitch_decoder(x)    # (batch, seq_len, 256)
        formant_out = self.formant_decoder(x)  # (batch, seq_len, 512)
        noise_out = self.noise_decoder(x)    # (batch, seq_len, 128)
        breath_out = self.breath_decoder(x)  # (batch, seq_len, 64)

        # Determine weights per task (broadcast over features)
        if task_weights is None:
            # Default equal weighting
            weights = torch.ones((x.size(0), 4), device=x.device)
        else:
            weights = task_weights

        # Apply weights and concatenate features, then fuse
        fused = torch.cat([
            pitch_out * weights[:, 0:1].unsqueeze(-1),
            formant_out * weights[:, 1:2].unsqueeze(-1),
            noise_out * weights[:, 2:3].unsqueeze(-1),
            breath_out * weights[:, 3:4].unsqueeze(-1)
        ], dim=-1)
        fused = self.fusion(fused)  # (batch, seq_len, d_model)

        # Final spectral reconstruction
        output_spectrum = self.spectral_decoder(fused)  # (batch, seq_len, 1024)

        return {
            'spectrum': output_spectrum,
            'pitch_features': pitch_out,
            'formant_features': formant_out,
            'noise_features': noise_out,
            'breath_features': breath_out
        }


class VocalDataset(torch.utils.data.Dataset):
    def __init__(self, data_dir: Path, sample_rate=44100, n_fft=2048, hop_length=256, win_length=1024, segment_frames=128):
        self.data_dir = Path(data_dir)
        self.sample_rate = sample_rate
        self.n_fft = n_fft
        self.hop_length = hop_length
        self.win_length = win_length
        self.segment_frames = segment_frames

        self.audio_files = list(self.data_dir.glob("**/*.wav")) + list(self.data_dir.glob("**/*.flac"))
        if len(self.audio_files) == 0:
            raise FileNotFoundError(f"No audio files found in {self.data_dir}")

    def __len__(self):
        return len(self.audio_files)

    def _to_spectrum(self, audio: np.ndarray) -> np.ndarray:
        # STFT magnitude, mapped to 1024 bins
        S = librosa.stft(audio, n_fft=self.n_fft, hop_length=self.hop_length, win_length=self.win_length, window='hann')
        mag = np.abs(S)
        # power to log scale, avoid log(0)
        mag = np.log1p(mag)
        # Ensure 1024 feature bins by padding/truncation along freq axis
        if mag.shape[0] < 1024:
            pad = 1024 - mag.shape[0]
            mag = np.pad(mag, ((0, pad), (0, 0)), mode='constant')
        elif mag.shape[0] > 1024:
            mag = mag[:1024, :]
        return mag.T  # (frames, 1024)

    def __getitem__(self, idx):
        audio_path = self.audio_files[idx]
        audio, sr = librosa.load(audio_path, sr=self.sample_rate, mono=True)

        spec = self._to_spectrum(audio)  # (frames, 1024)
        # Choose a random segment of fixed length
        if spec.shape[0] < self.segment_frames:
            # Pad frames
            pad_frames = self.segment_frames - spec.shape[0]
            spec = np.pad(spec, ((0, pad_frames), (0, 0)), mode='constant')
        else:
            start = np.random.randint(0, spec.shape[0] - self.segment_frames + 1)
            spec = spec[start:start + self.segment_frames]

        # Normalize per-sample
        mean = spec.mean(axis=0, keepdims=True)
        std = spec.std(axis=0, keepdims=True) + 1e-6
        spec_norm = (spec - mean) / std

        x = torch.tensor(spec_norm, dtype=torch.float32)  # (segment_frames, 1024)
        # For reconstruction training, target is original spectrum (pre-normalization optional). Here use normalized too.
        y = torch.tensor(spec_norm, dtype=torch.float32)

        return x, y


def train(model: nn.Module, dataloader, epochs: int, lr: float, device: torch.device, save_dir: Path):
    model.to(device)
    optimizer = torch.optim.Adam(model.parameters(), lr=lr)
    scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(optimizer, factor=0.5, patience=3)
    criterion = nn.MSELoss()

    save_dir.mkdir(parents=True, exist_ok=True)
    best_loss = float('inf')

    for epoch in range(1, epochs + 1):
        model.train()
        total_loss = 0.0
        for x, y in dataloader:
            x = x.to(device)  # (batch, frames, 1024)
            y = y.to(device)
            # Add batch dimension if needed
            # Model expects (batch, seq_len, 1024)
            outputs = model(x)
            pred = outputs['spectrum']
            loss = criterion(pred, y)

            optimizer.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
            optimizer.step()

            total_loss += loss.item() * x.size(0)

        avg_loss = total_loss / len(dataloader.dataset)
        scheduler.step(avg_loss)
        print(f"Epoch {epoch}/{epochs} - loss: {avg_loss:.6f}")

        # Save checkpoint
        ckpt_path = save_dir / f"vocal_repair_epoch{epoch}.pt"
        torch.save({'epoch': epoch, 'model_state_dict': model.state_dict(), 'loss': avg_loss}, ckpt_path)
        if avg_loss < best_loss:
            best_loss = avg_loss
            torch.save(model.state_dict(), save_dir / "vocal_repair_best.pt")


def collate_fn(batch):
    # Simple collate: stack tensors (already fixed length)
    xs, ys = zip(*batch)
    return torch.stack(xs, dim=0), torch.stack(ys, dim=0)


def main():
    parser = argparse.ArgumentParser(description="Train Vocal Repair Transformer")
    parser.add_argument("--data_dir", type=str, required=True, help="Directory containing wav/flac files")
    parser.add_argument("--epochs", type=int, default=5)
    parser.add_argument("--batch_size", type=int, default=8)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--num_workers", type=int, default=0)
parser.add_argument("--save_dir", type=str, default=str(Path("C:/Vocal Plugin/TitanVocal/Resources/Models")))
    args = parser.parse_args()

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Using device: {device}")

    dataset = VocalDataset(Path(args.data_dir))
    dataloader = torch.utils.data.DataLoader(dataset, batch_size=args.batch_size, shuffle=True, num_workers=args.num_workers, collate_fn=collate_fn)

    model = VocalRepairTransformer()
    save_dir = Path(args.save_dir)
    train(model, dataloader, args.epochs, args.lr, device, save_dir)

    print(f"Training complete. Best model saved to {save_dir / 'vocal_repair_best.pt'}")


if __name__ == "__main__":
    main()