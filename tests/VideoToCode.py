import cv2
import matplotlib.pyplot as plt

vidcap = cv2.VideoCapture ('Gachi.mp4')
outFile = '../tests/video_new.asm'

success, image = vidcap.read ()
dims = 100, 100

count = 0

vramState = [0] * (dims [0] * dims [1] * 3)

with open (outFile, 'w') as file:
    while success:

        if count >= 3000:
            exit (0)
        count += 1

        image = cv2.resize (image, dims, interpolation=cv2.INTER_LINEAR)
        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB) # remove for SMURRRRRFIKS
        rows, cols, _ = image.shape

        for i in range (rows):
            for j in range (cols):
                pixelAddress = (j * dims [0] + i) * 3
                r, g, b      = image [i, j]

                if vramState [pixelAddress] != r:
                    file.write (f"push {r}\npop [{pixelAddress}]\n")
                    vramState [pixelAddress] = r

                if vramState [pixelAddress + 1] != g:
                    file.write (f"push {g}\npop [{pixelAddress + 1}]\n")
                    vramState [pixelAddress + 1] = g

                if vramState [pixelAddress + 2] != b:
                    file.write (f"push {b}\npop [{pixelAddress + 2}]\n")
                    vramState [pixelAddress + 2] = b

        success, image = vidcap.read ()

        file.write ("sleep 25000\n");

    file.write ("hlt\n")


