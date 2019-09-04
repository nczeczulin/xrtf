from pkg_resources import get_distribution, DistributionNotFound
try:
    __version__ = get_distribution(__name__).version
except DistributionNotFound:
    # package is not installed
    pass
finally:
    del get_distribution, DistributionNotFound

from ._xrtf import Error
from ._xrtf import COMPRESSED, UNCOMPRESSED
from ._xrtf import compress, decompress, parse_header
