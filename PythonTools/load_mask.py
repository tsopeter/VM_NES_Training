
import torch
def load_torch_tensor(filename: str) -> torch.Tensor:
    d = torch.jit.load(filename)
    for z in d.parameters():
        return z