import numpy as np
def gs_algorithm(Image : np.array, iterations : int = 50)->np.array:
    Target = np.sqrt(Image)
    Object = np.random.rand(Target.shape[0], Target.shape[1])*2*np.pi

    phase = lambda p : np.divide(p, np.abs(p))
    Object = np.polar(np.ones_like(Object), Object)

    for _ in range(0, iterations):
        U      = np.fft.ifft2(Object)
        Up     = np.multiply(Target, phase(U))
        D      = np.fft.fft2(Up)
        Object = phase(D)

    return Object

def torch_gs_algorithm(Image : np.array, iterations : int = 50)->np.array:
    import torch
    Target = torch.sqrt(torch.from_numpy(Image))
    Object = torch.rand(Target.shape[0], Target.shape[1])*2*np.pi

    phase = lambda p : torch.divide(p, torch.abs(p))
    Object = torch.polar(torch.ones_like(Object), Object)

    for _ in range(0, iterations):
        U      = torch.fft.ifft2(Object)
        Up     = torch.multiply(Target, phase(U))
        D      = torch.fft.fft2(Up)
        Object = phase(D)

    return Object.cpu().numpy()