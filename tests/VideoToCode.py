import cv2

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
        rows, cols, _ = image.shape

        for i in range (rows):
            for j in range (cols):
                pixelAddress = (j * dims [0] + i) * 3
                pixel      = image [i, j]

                file.write (f"push {pixel [0]}\npop [{pixelAddress}]\npush {pixel [1]}\npop [{pixelAddress + 1}]\npush {pixel [2]}\npop [{pixelAddress + 2}]\n")

        success, image = vidcap.read ()

    file.write ("hlt\n")


