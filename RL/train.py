from re import A
import sys, time, os, math
from jpype import *
import numpy as np
from tuplegym import tupleSimEnv

curPath = os.path.abspath(os.path.dirname(__file__))
rootPath = os.path.split(curPath)[0]
sys.path.append(rootPath)
print(sys.path)

from d3qn import DQNAgent

def loop_d3qn(env):

    PERIOD_SAVE_MODEL = True
    a_dim = env.action_space.shape[0]
    s_dim = env.observation_space.shape[0]

    # print("a_dim:", a_dim, "s_dim:", s_dim)
    agent = DQNAgent(s_dim = s_dim, a_dim = a_dim)

    begin_time = time.time()
    decay_step = 0
    flag = 0
    for episode in range(300):
        ep_reward = 0
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

            # print("episode %s step %s"%(episode, i))
            print("obs_next:", next_state, "reward:", reward, "done:", done)
            print("actions:", actions)
            sys.stdout.flush()
            ep_reward += float(reward)

            if not done and reward != 0:
                agent.remember(state, action, reward, next_state, done)
                
            state = next_state
            # print(state)
            i += 1
            if done:
                # print("\nepisode %s: step %s, ep_reward %s, explore_probability %s"%(episode, i, ep_reward, explore_probability))
                sys.stdout.flush()

                if decay_step >= 30:
                    agent.update_target_model()

            agent.replay()
        end_time = time.time()
        if end_time - begin_time >= 600:
            exit()
 
        # if PERIOD_SAVE_MODEL and episode%20 == 0:
        #     model_name = "%s/model_%s.ckpt"%(MODEL_DIR, episode)
        #     agent.save(model_name)

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
