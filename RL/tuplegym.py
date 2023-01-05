from jpype import *
import os
from gym import Env, spaces
import numpy as np
import json
import math, sys, time
import socket

class tupleSimEnv(Env):
    def __init__(self, debug=True):
        self.NUM_TUPLE = 15
        self.UNIT_DIM = 6
        self.STATE_DIM = self.NUM_TUPLE * self.UNIT_DIM
        self.ACTION_DIM = 2 * (32 + 32 + 10)

        self.low_property = np.zeros((self.UNIT_DIM,))
        self.high_property = np.array([32, 32])
        
        self.observation_space = spaces.Box(0, 33, (self.STATE_DIM,))
        self.action_space = spaces.Box(0, 33, (self.ACTION_DIM,))

        self.__initialize()

        self.debug = debug

    def __initialize(self):
        # self.context = socket.socket()
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect(("localhost", 8888))

    def step(self, action):
        self.socket.send(action.encode("gbk"))

        res = self.socket.recv(1024)
        res = res.decode("gbk").split("_")
        obs = [int(i) for i in res[0:-3]]
        cost_old = float(res[-3])
        cost_new = float(res[-2])
        done = (int(res[-1]) == 1)
        # reward = math.log10(abs(cost_old - cost_new) + 1)
        # if cost_old < cost_new:
        #     reward = -reward
        # reward = cost_old - cost_new / 200
        
        reward = abs((cost_old - cost_new) / cost_old)
        if reward > 1:
            reward = 1
        elif reward <= 0.5:
            reward = 0.5 - 2 * (0.5 - reward) * (0.5 - reward)
        elif reward > 0.5:
            reward = 0.5 + 2 * (reward - 0.5) * (reward - 0.5)
        
        if cost_old < cost_new:
            reward = -reward
        return obs, reward, done
    
    
    def reset(self):
        # self.__initialize()
        self.socket.send("2_0_0".encode("gbk"))
        res = self.socket.recv(1024)
        res = res.decode("gbk").split("_")
        obs = [int(i) for i in res[0:-1]]
        return obs
    
    def getMask(self, state):
        if state[1] == 0:
            return [0] * 74 + [1] * 74
        if state[3] != 0:
            return [1] * 74 + [0] * 74

        mask = [1] * 148
        used_field = []
        if state[1] != 0:
            used_field.append(state[0])
        if state[3] != 0:
            used_field.append(state[2])
        
        for field in used_field:
            if field == 0:
                # for i in range(32): # 0 - 31
                #     mask[i] = 0
                for i in range(74, 74 + 32): # 74 - 105
                    mask[i] = 0
            elif field == 1:
                # for i in range(32, 64): # 32 - 63
                #     mask[i] = 0
                for i in range(74 + 32, 74 + 64): # 106 - 137
                    mask[i] = 0
            else:
                mask[136 + field] = 0 # 138 - 147
        
        for i in range(0, 15):
            ii = i * 6
            f1 = state[ii]
            l1 = state[ii + 1]
            l2 = state[ii + 3]
            if l1 == 0:
                break
            if l2 == 0:
                if f1 == 0:
                    mask[l1 - 1] = 0
                elif f1 == 1:
                    mask[32 + l1 - 1] = 0
                else:
                    mask[64 + f1 - 2] = 0 

        return mask
    
    def render(self):
        pass
    
    def close(self):
        pass
    
    def render(self):
        pass
    
    def close(self):
        pass