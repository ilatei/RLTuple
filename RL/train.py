from re import A
import sys, time, os, math
from jpype import *
import numpy as np
from tuplegym import tupleSimEnv
import matplotlib.pyplot as plt

curPath = os.path.abspath(os.path.dirname(__file__))
rootPath = os.path.split(curPath)[0]
sys.path.append(rootPath)
print(sys.path)

from d3qn import DQNAgent

def calR(ep_reward):
    gamma = 0.7
    ret = 0
    for i in range(len(ep_reward)):
        ret = ret + ep_reward[i] * pow(gamma, i)
    return ret

def shapeR(r_origin, r_modified, reward):
    l = 0
    r = len(r_origin) - 1
    while l < r:
        m = int((l + r) / 2)
        if r_origin[m] < reward:
            l = m + 1
        elif r_origin[m] > reward:
            r = m - 1
        else:
            return r_modified[m]
    return r_modified[l]


def collectR(env, agent, episode = 50):
    rewards = []
    for _ in range(episode):
        state = env.reset()
        done = False
        while not done:
            mask = env.getMask(state)
            action, explore_probability = agent.act(state, 0, mask)
            flag = int(action/74)
            field_ = action % 74
            field = 0
            len = 0
            if field_ <= 31:
                field = 0
                len = field_ + 1
            elif field_ <= 63:
                field = 1
                len = field_ - 31
            else:
                field = field_ - 64 + 2

            if field == 2 or field == 3 or field == 8:
                len = 16
            elif field == 4 or field == 11:
                len = 8
            elif field == 5:
                len = 32;
            elif field == 6 or field == 7:
                len = 48;
            elif field == 9:
                len = 12;
            elif field == 10:
                len = 3;

            actions = str(flag) + "_" + str(field) + "_" + str(len)
            next_state, reward, done = env.step(actions)
            rewards.append(float(reward))
            state = next_state
    
    heights, bins = np.histogram(rewards, bins=100)
    # heights = float(heights)
    sum1 = float(sum(heights[0:50]))
    sum2 = float(sum(heights[50:]))
    heights = np.array(heights, dtype=float)
    for i in range(50):
        heights[i] = (float)(heights[i]) / sum1
        heights[i + 50] = heights[i + 50] /  sum2
    
    for i in range(1, 100):
        heights[i] = heights[i] + heights[i-1]
    heights = heights - 1

    return bins[:-1], heights


def evaluate(env, agent):
    ep_reward = []
    state = env.reset()
    done = False
    while not done:
        mask = env.getMask(state)
        action, explore_probability = agent.act(state, 10000000, mask)

        flag = int(action/74)
        field_ = action % 74
        field = 0
        len = 0
        if field_ <= 31:
            field = 0
            len = field_ + 1
        elif field_ <= 63:
            field = 1
            len = field_ - 31
        else:
            field = field_ - 64 + 2

        if field == 2 or field == 3 or field == 8:
            len = 16
        elif field == 4 or field == 11:
            len = 8
        elif field == 5:
            len = 32;
        elif field == 6 or field == 7:
            len = 48;
        elif field == 9:
            len = 12;
        elif field == 10:
            len = 3;

        actions = str(flag) + "_" + str(field) + "_" + str(len)

        next_state, reward, done = env.step(actions)

            # print("episode %s step %s"%(episode, i))
            # print("obs_next:", next_state, "reward:", reward, "done:", done)
            # print("actions:", actions)
        sys.stdout.flush()
        ep_reward.append(float(reward))
        state = next_state
            
        if done:
            print("ep_reward %s %s" %(calR(ep_reward), str(ep_reward)))
            sys.stdout.flush()



def loop_d3qn(env):
    PERIOD_SAVE_MODEL = True
    reward_shaping = True
    a_dim = env.action_space.shape[0]
    s_dim = env.observation_space.shape[0]

    # print("a_dim:", a_dim, "s_dim:", s_dim)
    agent = DQNAgent(s_dim = s_dim, a_dim = a_dim)

    if reward_shaping:
        r_origin, r_modified =  collectR(env, agent, 50)

    begin_time = time.time()
    decay_step = 0
    flag = 0
    for episode in range(1000):
        ep_reward = []
        ep_time = time.time()

        state = env.reset()
        # print(state)
        done = False
        i = 0
        while not done:
            decay_step += 1
            mask = env.getMask(state)
            action, explore_probability = agent.act(state, decay_step, mask)

            flag = int(action/74)
            field_ = action % 74
            field = 0
            len = 0
            if field_ <= 31:
                field = 0
                len = field_ + 1
            elif field_ <= 63:
                field = 1
                len = field_ - 31
            else:
                field = field_ - 64 + 2

            if field == 2 or field == 3 or field == 8:
                len = 16
            elif field == 4 or field == 11:
                len = 8
            elif field == 5:
                len = 32;
            elif field == 6 or field == 7:
                len = 48;
            elif field == 9:
                len = 12;
            elif field == 10:
                len = 3;

            actions = str(flag) + "_" + str(field) + "_" + str(len)
            # print(state)
            # print(actions)

            # actions = input()

            next_state, reward, done = env.step(actions)
            
            if reward_shaping:
                reward = shapeR(r_origin, r_modified, reward)


            # print("episode %s step %s"%(episode, i))
            # print("obs_next:", next_state, "reward:", reward, "done:", done)
            # print("actions:", actions)
            sys.stdout.flush()
            ep_reward.append(float(reward))

            if not done and reward != 0:
                agent.remember(state, action, reward, next_state, done)
                
            state = next_state
            # print(state)
            i += 1
            if done:
                # print("\nepisode %s: step %s, ep_reward %s, explore_probability %s"%(episode, i, sum(ep_reward), explore_probability))
                sys.stdout.flush()
                if episode % 5 == 0:
                    # print("explore_probability %s" %(explore_probability))
                    evaluate(env, agent)

                if decay_step >= 30:
                    agent.update_target_model()

            agent.replay()
        end_time = time.time()
        # if end_time - begin_time >= 60:
        #     print(decay_step)
        #     print(episode)
        #     exit()

    env.close()
    # print("Game is over!")


if __name__ == "__main__":
    # if not os.path.exists("./log/"):
    #     os.mkdir("./log/")
    # file = open(LOG_FILE, "w")
    # sys.stdout = file

    env = tupleSimEnv()

    # record
    # print("training begins: %s"%(time.asctime(time.localtime(time.time()))))
    # sys.stdout.flush()
    loop_d3qn(env)
