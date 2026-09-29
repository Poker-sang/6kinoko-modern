"""Original cyan arrow; no original game pixels. Run to regenerate CV2."""
import struct
from pathlib import Path
pixels=bytearray(64*32*4)
for y in range(32):
    for x in range(64):
        if 6<=x<=26-abs(y-16) and 4<=y<=28:
            at=(y*64+x)*4;pixels[at:at+4]=bytes((220,230,45,255))
p=Path(__file__).parent/'data/system/title/title_cursor.cv2'
p.parent.mkdir(parents=True,exist_ok=True)
p.write_bytes(struct.pack('<BIIII',32,64,32,64,0)+pixels)
