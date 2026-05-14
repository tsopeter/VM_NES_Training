import numpy as np
import torch
def propagate(Image  : np.ndarray, 
              z      : float,
              Height : float = 9.019521960974117e-3,    # m
              Width  : float = 14.431235137558588e-3,
              PixelH : int   = 800,   # px
              PixelW : int   = 1280,
              Lambda : float = 633e-9 # 633 nm
              )->np.ndarray:
    dh     = PixelH/Height
    dw     = PixelW/Width
    k      = 2 * np.pi / Lambda
    F      = np.fft.fftshift(np.fft.fft2(Image))

    xi  = np.linspace(-dh/2, dh/2, PixelH) * Lambda
    eta = np.linspace(-dw/2,dw/2, PixelW) * Lambda

    Xi, Eta = np.meshgrid(xi, eta)
    kz     = k * np.sqrt(
        1 - Xi.T**2 - Eta.T**2
    )
    return np.fft.ifft2(
        np.fft.ifftshift(
            F * np.exp(1j * kz * z)
        )
    )

ANGULAR_SPECTRUM_METHOD_CACHED_F = None
ANGULAR_SPECTRUM_METHOD_CACHED_PROPERTIES = {
    'Height': None,
    'Width' : None,
    'PixelH': None,
    'PixelW': None,
    'Lambda': None,
    'Z' : None,
    'Warning_Silenced': False
}

def is_angular_spectrum_method_cached():
    global ANGULAR_SPECTRUM_METHOD_CACHED_F
    return ANGULAR_SPECTRUM_METHOD_CACHED_F is not None

def clear_angular_spectrum_method_cache():
    global ANGULAR_SPECTRUM_METHOD_CACHED_F
    global ANGULAR_SPECTRUM_METHOD_CACHED_PROPERTIES
    ANGULAR_SPECTRUM_METHOD_CACHED_F = None
    ANGULAR_SPECTRUM_METHOD_CACHED_PROPERTIES = {
        'Height': None,
        'Width' : None,
        'PixelH': None,
        'PixelW': None,
        'Lambda': None,
        'Z' : None,
        'Warning_Silenced': False
    }

def silence_angular_spectrum_method_cache_warning():
    global ANGULAR_SPECTRUM_METHOD_CACHED_PROPERTIES
    ANGULAR_SPECTRUM_METHOD_CACHED_PROPERTIES['Warning_Silenced'] = True

def torch_propagate_no_grad (Image  : torch.Tensor,
                             z      : float,
                             Height : float = 9.019521960974117e-3,    # m
                             Width  : float = 14.431235137558588e-3,
                             PixelH : int   = 800,   # px
                             PixelW : int   = 1280,
                             Lambda : float = 633e-9 # 633 nm
              )->torch.Tensor:
    with torch.no_grad():
        return torch_propagate(Image, z, Height, Width, PixelH, PixelW, Lambda)

def torch_propagate(Image  : torch.Tensor, 
              z      : float,
              Height : float = 9.019521960974117e-3,    # m
              Width  : float = 14.431235137558588e-3,
              PixelH : int   = 800,   # px
              PixelW : int   = 1280,
              Lambda : float = 633e-9, # 633 nm
              cache  : torch.Tensor = None
              ):
    # If Image is [H, W] or [B, H, W], reshape
    # to [B, 1, H, W]
    old_shape = Image.shape
    device    = Image.device
    if Image.dim() == 2:
        Image = Image.unsqueeze(0).unsqueeze(0)
    elif Image.dim() == 3:
        Image = Image.unsqueeze(1)
    elif Image.dim() == 4:
        pass

    if cache is None:
        '''
        dh     = PixelH/Height
        dw     = PixelW/Width
        k      = 2 * np.pi / Lambda

        xi  = torch.linspace(-dh/2, dh/2, PixelH, device=device) * Lambda
        eta = torch.linspace(-dw/2,dw/2, PixelW, device=device) * Lambda
        '''

        dh = Height / PixelH
        dw = Width  / PixelW

        k = 2 * np.pi / Lambda
        fx = torch.fft.fftfreq(PixelH, d=dh).to(device)
        fy = torch.fft.fftfreq(PixelW, d=dw).to(device)
        fx, fy = torch.fft.fftshift(fx), torch.fft.fftshift(fy)
        FX, FY = torch.meshgrid(fy, fx, indexing='xy')

        kx = 2 * np.pi * FX
        ky = 2 * np.pi * FY
        kz = torch.sqrt((k ** 2 - kx ** 2 - ky ** 2).clamp(min=0.0))

        '''
        Xi, Eta = torch.meshgrid(xi, eta)
        kz     = k * torch.sqrt(
            1 - Xi.T**2 - Eta.T**2
        )
        '''

        cache = torch.exp(1j * kz * z).to(device)

    F = torch.fft.fftshift(torch.fft.fft2(Image, dim=(-2, -1)), dim=(-2, -1))
    
    return torch.fft.ifft2(
        torch.fft.ifftshift(
            F * cache
            , dim=(-2, -1)
        )
    ).reshape(old_shape), cache

