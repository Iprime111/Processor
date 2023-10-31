import cv2
import matplotlib.pyplot as plt

vidcap = cv2.VideoCapture ('Gachi.mp4')
outFile = '../tests/video.asm'

success, image = vidcap.read ()
dims = 100, 100

count = 0



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

                file.write (f"push {r}\npop [{pixelAddress}]\npush {g}\npop [{pixelAddress + 1}]\npush {b}\npop [{pixelAddress + 2}]\n")

        success, image = vidcap.read ()

    file.write ("hlt\n")


