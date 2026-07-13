import argparse
from pathlib import Path

import torch
import torch.nn as nn
import torch.optim as optim


class TinyDenoiser(nn.Module):
    def __init__(self):
        super().__init__()
        self.net = nn.Sequential(
            nn.Conv2d(3, 32, 3, padding=1),
            nn.ReLU(inplace=True),
            nn.Conv2d(32, 32, 3, padding=1),
            nn.ReLU(inplace=True),
            nn.Conv2d(32, 3, 3, padding=1),
        )

    def forward(self, x):
        return self.net(x)


def make_batch(batch_size: int, h: int, w: int):
    clean = torch.rand(batch_size, 3, h, w)
    noise = 0.15 * torch.randn_like(clean)
    noisy = torch.clamp(clean + noise, 0.0, 1.0)
    return noisy, clean


def train(epochs: int, lr: float, out_dir: Path):
    out_dir.mkdir(parents=True, exist_ok=True)

    model = TinyDenoiser()
    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=lr)

    model.train()
    for epoch in range(1, epochs + 1):
        noisy, clean = make_batch(batch_size=16, h=64, w=64)
        pred = model(noisy)
        loss = criterion(pred, clean)

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()

        if epoch % 10 == 0 or epoch == 1:
            print(f"epoch={epoch} loss={loss.item():.6f}")

    model_path = out_dir / "tiny_denoiser.pt"
    torch.save(model.state_dict(), model_path)
    print(f"Saved model to {model_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--epochs", type=int, default=100)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--out", type=Path, default=Path("artifacts"))
    args = parser.parse_args()

    train(args.epochs, args.lr, args.out)
