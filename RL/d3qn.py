# Tutorial by www.pylessons.com
# Tutorial written for - Tensorflow 1.15, Keras 2.2.4

import os
import random
import gym
import pylab
import numpy as np
from collections import deque
from keras.models import Model, load_model
from keras.layers import Input, Dense, Lambda, Add
from keras.optimizers import Adam, RMSprop
from keras import backend as K
import tensorflow as tf

def OurModel(input_shape, action_space, dueling):
    X_input = Input(input_shape)
    X = X_input

    # 'Dense' is the basic form of a neural network layer
    # Input Layer of state size(4) and Hidden Layer with 512 nodes
    X = Dense(256, input_shape=input_shape, activation="relu", kernel_initializer='he_uniform')(X)

    # # Hidden layer with 256 nodes
    # X = Dense(256, activation="relu", kernel_initializer='he_uniform')(X)
    
    # Hidden layer with 64 nodes
    X = Dense(64, activation="relu", kernel_initializer='he_uniform')(X)

    if dueling:
        state_value = Dense(1, kernel_initializer='he_uniform')(X)
        state_value = Lambda(lambda s: K.expand_dims(s[:, 0], -1), output_shape=(action_space,))(state_value)

        action_advantage = Dense(action_space, kernel_initializer='he_uniform')(X)
        action_advantage = Lambda(lambda a: a[:, :] - K.mean(a[:, :], keepdims=True), output_shape=(action_space,))(action_advantage)

        X = Add()([state_value, action_advantage])
    else:
        # Output Layer with # of actions: 2 nodes (left, right)
        X = Dense(action_space, activation="linear", kernel_initializer='he_uniform')(X)

    model = Model(inputs = X_input, outputs = X, name='CartPole Dueling DDQN model')
    # model.compile(loss="mean_squared_error", optimizer=RMSprop(lr=0.00025, rho=0.95, epsilon=0.01), metrics=["accuracy"])
    model.compile(loss="mean_squared_error", optimizer=Adam(lr=0.0003), metrics=["accuracy"])

    model.summary()
    return model

class DQNAgent:
    def __init__(self, s_dim, a_dim):
        # self.env_name = env_name       
        # self.env = gym.make(env_name)
        # self.env.seed(0)  
        # by default, CartPole-v1 has max episode steps = 500
        # self.env._max_episode_steps = 4000
        # self.state_size = self.env.observation_space.shape[0]
        # self.action_size = self.env.action_space.n
        # self.sess = tf.Session()
        # self.sess.run(tf.global_variables_initializer())
        self.state_size = s_dim
        self.action_size = a_dim

        self.EPISODES = 1000
        self.memory_up = deque(maxlen=200)
        self.memory_down = deque(maxlen=200)
        self.gamma = 0.7    # discount rate

        # EXPLORATION HYPERPARAMETERS for epsilon and epsilon greedy strategy
        self.epsilon = 1.0 # exploration probability at start
        self.epsilon_min = 0.01 # minimum exploration probability
        self.epsilon_decay = 0.0005 # exponential decay rate for exploration prob
        
        self.batch_size = 32

        # defining model parameters
        self.multipool = True
        self.ddqn = True # use double deep q network
        self.Soft_Update = True # use soft parameter update
        self.dueling = True # use dealing network
        self.epsilon_greedy = True # use epsilon greedy strategy

        self.TAU = 0.1 # target network soft update hyperparameter

        self.model = OurModel(input_shape=(self.state_size,), action_space = self.action_size, dueling = self.dueling)
        self.target_model = OurModel(input_shape=(self.state_size,), action_space = self.action_size, dueling = self.dueling)

    # after some time interval update the target model to be same with model
    def update_target_model(self):
        if not self.Soft_Update and self.ddqn:
            self.target_model.set_weights(self.model.get_weights())
            return
        if self.Soft_Update and self.ddqn:
            q_model_theta = self.model.get_weights()
            target_model_theta = self.target_model.get_weights()
            counter = 0
            for q_weight, target_weight in zip(q_model_theta, target_model_theta):
                target_weight = target_weight * (1-self.TAU) + q_weight * self.TAU
                target_model_theta[counter] = target_weight
                counter += 1
            self.target_model.set_weights(target_model_theta)

    def remember(self, state, action, reward, next_state, done):
        experience = state, action, reward, next_state, done
        if self.multipool:
            if reward >= 0.5 or reward <= -0.5:
                self.memory_up.append((experience))
            else:
                self.memory_down.append((experience))
        else:
            self.memory_up.append((experience))

    def act(self, state, decay_step, mask):
        # EPSILON GREEDY STRATEGY
        if self.epsilon_greedy:
        # Here we'll use an improved version of our epsilon greedy strategy for Q-learning
            explore_probability = self.epsilon_min + (self.epsilon - self.epsilon_min) * np.exp(-self.epsilon_decay * decay_step)
        # OLD EPSILON STRATEGY
        else:
            if self.epsilon > self.epsilon_min:
                self.epsilon *= (1-self.epsilon_decay)
            explore_probability = self.epsilon

        if explore_probability > np.random.rand():
            # Make a random action (exploration)
            while(True):
                _action = random.randrange(self.action_size)
                if mask[_action] == 1:
                    break
            return _action, explore_probability
        else:
            # Get action from Q-network (exploitation)
            # Estimate the Qs values state
            # Take the biggest Q value (= the best action)
            probility = self.model.predict(np.array(state).reshape(-1, self.state_size))
            for i in range(mask.__len__()):
                if mask[i] == 0:
                    probility[0][i] = -9999
            return np.argmax(probility), explore_probability

    def replay(self):
        if self.multipool and (len(self.memory_up) < self.batch_size * 0.5 or len(self.memory_down) < self.batch_size * 0.5):
            return
        if (not self.multipool) and (len(self.memory_up) < self.batch_size):
            return
        # Randomly sample minibatch from the memory
        if self.multipool:
            batch_size_up = int(self.batch_size * 0.5)
            batch_size_down = int(self.batch_size * 0.5)
        else:
            batch_size_up = int(self.batch_size)
            batch_size_down = 0

        minibatch_up = random.sample(self.memory_up, batch_size_up)
        minibatch_down = random.sample(self.memory_down, batch_size_down)

        state = np.zeros((self.batch_size, self.state_size))
        next_state = np.zeros((self.batch_size, self.state_size))
        action, reward, done = [], [], []

        # do this before prediction
        # for speedup, this could be done on the tensor level
        # but easier to understand using a loop
        i = 0
        for _ in range(batch_size_up):
            state[i] = minibatch_up[i][0]
            action.append(minibatch_up[i][1])
            reward.append(minibatch_up[i][2])
            next_state[i] = minibatch_up[i][3]
            done.append(minibatch_up[i][4])
            i += 1

        for _ in range(batch_size_down):
            state[i] = minibatch_down[i - batch_size_up][0]
            action.append(minibatch_down[i - batch_size_up][1])
            reward.append(minibatch_down[i - batch_size_up][2])
            next_state[i] = minibatch_down[i - batch_size_up][3]
            done.append(minibatch_down[i - batch_size_up][4])
            i += 1

        # do batch prediction to save speed
        # predict Q-values for starting state using the main network
        target = self.model.predict(state)
        # predict best action in ending state using the main network
        target_next = self.model.predict(next_state)
        # predict Q-values for ending state using the target network
        target_val = self.target_model.predict(next_state)

        for i in range(len(minibatch_up) + len(minibatch_down)):
            # correction on the Q value for the action used
            if done[i]:
                target[i][action[i]] = reward[i]
            else:
                if self.ddqn: # Double - DQN
                    # current Q Network selects the action
                    # a'_max = argmax_a' Q(s', a')
                    a = np.argmax(target_next[i])
                    # target Q Network evaluates the action
                    # Q_max = Q_target(s', a'_max)
                    target[i][action[i]] = reward[i] + self.gamma * (target_val[i][a])   
                else: # Standard - DQN
                    # DQN chooses the max Q value among next actions
                    # selection and evaluation of action is on the target Q Network
                    # Q_max = max_a' Q_target(s', a')
                    target[i][action[i]] = reward[i] + self.gamma * (np.amax(target_next[i]))

        # Train the Neural Network with batches
        self.model.fit(state, target, batch_size=self.batch_size, verbose=0)


    def load(self, name):
        self.model = load_model(name)

    def save(self, name):
        self.model.save(name)

    # pylab.figure(figsize=(18, 9))
    # def PlotModel(self, score, episode):
    #     self.scores.append(score)
    #     self.episodes.append(episode)
    #     self.average.append(sum(self.scores[-50:]) / len(self.scores[-50:]))
    #     pylab.plot(self.episodes, self.average, 'r')
    #     pylab.plot(self.episodes, self.scores, 'b')
    #     pylab.ylabel('Score', fontsize=18)
    #     pylab.xlabel('Steps', fontsize=18)
    #     dqn = 'DQN_'
    #     softupdate = ''
    #     dueling = ''
    #     greedy = ''
    #     if self.ddqn: dqn = 'DDQN_'
    #     if self.Soft_Update: softupdate = '_soft'
    #     if self.dueling: dueling = '_Dueling'
    #     if self.epsilon_greedy: greedy = '_Greedy'
    #     try:
    #         pylab.savefig(dqn+self.env_name+softupdate+dueling+greedy+".png")
    #     except OSError:
    #         pass

    #     return str(self.average[-1])[:5]
    
    def run(self):
        decay_step = 0
        for e in range(self.EPISODES):
            state = self.env.reset()
            state = np.reshape(state, [1, self.state_size])
            done = False
            i = 0
            while not done:
                # self.env.render()
                decay_step += 1
                action, explore_probability = self.act(state, decay_step)
                next_state, reward, done, _ = self.env.step(action)
                next_state = np.reshape(next_state, [1, self.state_size])
                if not done or i == self.env._max_episode_steps-1:
                    reward = reward
                else:
                    reward = -100
                self.remember(state, action, reward, next_state, done)
                state = next_state
                i += 1
                if done:
                    self.update_target_model()

                self.replay()

    def test(self):
        self.load(self.Model_name)
        for e in range(self.EPISODES):
            state = self.env.reset()
            state = np.reshape(state, [1, self.state_size])
            done = False
            i = 0
            while not done:
                self.env.render()
                action = np.argmax(self.model.predict(state))
                next_state, reward, done, _ = self.env.step(action)
                state = np.reshape(next_state, [1, self.state_size])
                i += 1
                if done:
                    print("episode: {}/{}, score: {}".format(e, self.EPISODES, i))
                    break

if __name__ == "__main__":
    env_name = 'CartPole-v1'
    agent = DQNAgent(env_name)
    agent.run()
    #agent.test()