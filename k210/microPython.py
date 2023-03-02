import Maix, gc, time
from fpioa_manager import fm
from machine import UART
# from board import board_info
import sensor, image
import KPU as kpu

# ********************************************************************************************

class Position:
    def __init__(self, x, y, w, h):
        self.x = x
        self.y = y
        self.width = w
        self.height = h

class AiConfig:
    def __init__(self):
        self.digitCount = 0
        self.invert = False
        self.positions = None


def printMem(msg = ""):
    print("\n$ mem report", msg)
    print("heap free mem:", Maix.utils.heap_free() / 1024, "kb")
    print("gc free mem:", gc.mem_free() / 1024, "kb", end="\n\n")

# ********************************************************************************************

print("initializing resources")
clock = time.clock()

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.set_vflip(1)
sensor.set_hmirror(1)
sensor.skip_frames(time = 2000)

img = sensor.snapshot()
img.to_grayscale(False)
arr = bytes(img)
arr_size = len(arr)
print("image size:", arr_size)

fm.register(21, fm.fpioa.UART1_TX, force=True)
fm.register(22, fm.fpioa.UART1_RX, force=True)
uartHandle = UART(
    UART.UART1, 921600, 8, 0, 0, timeout=1000, read_buf_len=4096)
time.sleep_ms(100) # wait till uart is ready

print("loading kmodel")
task = kpu.load(0x400000)
kpu.set_outputs(task, 0, 10, 1, 1)

# this will store the settings
aiConfig = AiConfig()
printMem("after initialization")

# ********************************************************************************************

def getSubImg(index, parentImg):
    imgSize = max(
        aiConfig.positions[index].width,
        aiConfig.positions[index].height
    )

    # calculate subImg position
    left = (imgSize - aiConfig.positions[index].width) // 2
    right = imgSize - aiConfig.positions[index].width - left
    top = (imgSize - aiConfig.positions[index].height) // 2
    bottom = imgSize - aiConfig.positions[index].height - top

    # crop the subImg
    subImg = parentImg.copy(roi=(
        aiConfig.positions[index].x - left,
        aiConfig.positions[index].y - top,
        imgSize, imgSize
    ))

    if aiConfig.invert:
        subImg.invert()

    # binarize the subImg using dynamic thresholding
    subImg.strech_char(1)
    th = subImg.get_histogram().get_threshold().value()
    subImg.binary([(th, 256)], invert=False, copy=False)

    # make sure that background is black outside ROI
    subImg.draw_rectangle(0, 0, left, imgSize, 0, fill=True)
    subImg.draw_rectangle(
        aiConfig.positions[index].width+left+1, 0,
        right, imgSize, 0, fill=True
    )
    subImg.draw_rectangle(0, 0, imgSize, top, 0, fill=True)
    subImg.draw_rectangle(
        0, aiConfig.positions[index].height+top+1,
        imgSize, bottom, 0, fill=True
    )

    return subImg.resize(28, 28)

def aiProcess():
    print("performing neural net inference")
    parentImg = sensor.snapshot()
    parentImg.to_grayscale(0)
    number = ""
    probability = 1

    for digit in range(aiConfig.digitCount):
        subImg = getSubImg(digit, parentImg)
        subImg.pix_to_ai()
        fmap = kpu.forward(task, subImg)
        plist = fmap[:]
        pmax = max(plist)
        max_index = plist.index(pmax)
        number += str(max_index)
        probability *= pmax

    print("infered", number, "with probability", pmax)
    return number

# ********************************************************************************************

lastPoint = 0

def camTransmitter(args):

    try: sendSize = int(args[1])
    except: return False

    global lastPoint
    tmpIndx = lastPoint

    if args[0] == "camStart":
        tmpIndx = 0
        if arr_size <= sendSize:
            uartHandle.write(arr)
            lastPoint = arr_size
        else:
            uartHandle.write(arr[:sendSize])
            lastPoint = sendSize


    elif args[0] == "camNext":
        if lastPoint+sendSize < arr_size:
            uartHandle.write(arr[lastPoint: lastPoint+sendSize])
            lastPoint += sendSize

        elif lastPoint < arr_size:
            uartHandle.write(arr[lastPoint:])
            lastPoint = arr_size

    else: return False

    print("Sending", lastPoint-tmpIndx, "bytes")    
    return True

# ********************************************************************************************

def configHandler(args):

    global aiConfig

    if not args[0] == "config": return False

    try:
        aiConfig.digitCount = int(args[1])
        print("digitCount:", aiConfig.digitCount)
        aiConfig.invert = bool(int(args[2]))
        print("invert:", aiConfig.invert)

        aiConfig.positions = [None]*aiConfig.digitCount
        for digit in range(aiConfig.digitCount):
            index = digit*4 + 3
            x = int(args[index])
            y = int(args[index+1])
            w = int(args[index+2])
            h = int(args[index+3])
            aiConfig.positions[digit] = Position(x, y, w, h)
            print("Position", digit, ":", x, y, w, h)

    except: return False
    uartHandle.write("recieved")

    aiConfig.positions.sort(
        key = lambda tmp: tmp.x,
        reverse = False
    )

    return True

# ********************************************************************************************

print("-"*50)
last_read = ""

while True:
    if uartHandle.any():
        read_data = uartHandle.read()
        input_str = read_data.decode()
        inputs = input_str.split(':')
        print("\nCommand:", inputs[0])
        clock.tick()

        if(inputs[0] == "camTakeNew"):
            img = sensor.snapshot()
            img.to_grayscale(0)
            arr = bytes(img)
            gc.collect()

        elif camTransmitter(inputs): pass    
        elif configHandler(inputs): pass    

        elif (inputs[0] == "AIread"):
            last_read = aiProcess()
            gc.collect()
            uartHandle.write("done")

        elif (inputs[0] == "AIsend"):
            result = "num:" + last_read + ";"
            print("sending", result)
            uartHandle.write(result)

        print(1/clock.fps())
    time.sleep_ms(100)

uartHandle.deinit()
del uartHandle
