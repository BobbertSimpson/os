import threading
import time

lock = threading.Lock()
cond1 = threading.Condition(lock)
ready = False

def producer():
    global ready
    with cond1:
        if ready:
            return

        ready = True
        print("provided")
        cond1.notify()

def consumer():
    global ready
    with cond1:
        while not ready:
            cond1.wait()
            print("awoke")

        ready = False
        print("consumed")

# Example usage
if __name__ == "__main__":
    # Create threads
    producer_thread = threading.Thread(target=producer)
    consumer_thread = threading.Thread(target=consumer)

    # Start threads
    consumer_thread.start()
    time.sleep(3)
    producer_thread.start()

    # Wait for threads to finish
    producer_thread.join()
    consumer_thread.join()

