import sys
from time import time
sys.setrecursionlimit(10000)

def ack(m, n):
    if m > 0:
        if n == 0:
            return ack(m - 1, 1)
        else:
            return ack(m - 1, ack(m, n - 1))
    else:
        return n + 1



def ack2(m, n):
    stack = [m]
    
    while stack:
        m = stack.pop()
        if m == 0:
            if not stack:
                return n + 1
            n += 1
        elif n == 0:
            stack.append(m - 1)
            n = 1
        else:
            stack.append(m - 1)
            stack.append(m)
            n -= 1

    return n


if __name__ == '__main__':
    tic = time()
    print(ack(3, 7))
    print(time()-tic)
    tic = time()
    print(ack(3, 7))
    print(time()-tic)