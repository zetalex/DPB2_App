import zmq
import json
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as md
import sys
import dateutil
import datetime


def get_environment_magnitude(json_data,board,magnitude):
    # Parse JSON
    json_obj = json.loads(json_data)
    data = json_obj
    # Extract info from data field
    dpb_data = data[board]
    try:
        magnitude_value = dpb_data['fpgatemp']
        return magnitude_value
    except:
        return 0
    
def plot_magnitude(x,y):
    dates = [dateutil.parser.parse(s) for s in x]

    plt_data = range(5,9)
    plt.subplots_adjust(bottom=0.2)
    plt.xticks( rotation=25 )

    ax=plt.gca()
    ax.set_xticks(dates)

    xfmt = md.DateFormatter('%Y-%m-%d %H:%M:%S')
    ax.xaxis.set_major_formatter(xfmt)
    plt.title("Magnitude Plot")
    plt.xlabel("Time (date)")
    plt.ylabel("Magnitude (units)")
    plt.plot(dates,y, "o-")
    plt.show()

def main():
    ip_str = "tcp://" + str(sys.argv[1]) + ":5555"
    num_points=int(sys.argv[2])
    magnitude = np.tile(np.float32(0),num_points)
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    
    socket.setsockopt_string(zmq.SUBSCRIBE, "")
    socket.connect(ip_str)
    
    print("Establishing connection with DPB at address " + str(str(sys.argv[1])))
    json_data = socket.recv_string()
    print(json.dumps(json.loads(json_data), indent=4))
    
    datestrings = ["" for x in range(num_points)]
    
    for i in range(0, num_points):
        json_data = socket.recv_string()
        magnitude[i] = get_environment_magnitude(json_data,"DPB","fpgatemp")
        datestrings[i] = str(datetime.datetime.now())
        print(datestrings[i])
    plot_magnitude(datestrings,magnitude)

if __name__ == "__main__":
    main()
