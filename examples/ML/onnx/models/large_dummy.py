import torch
from torch import nn


class LargeDummy(nn.Module):

    def __init__(self):
        super().__init__()

        self.input_size = 64
        self.output_size = 64

        self.fc1 = nn.Linear(self.input_size, 500)
        self.conv1 = nn.Conv1d(500, 500, 
                              kernel_size=3,
                              stride=1,
                              padding=0
                              )
        self.conv2 = nn.Conv1d(500, 500, 
                              kernel_size=3,
                              stride=1,
                              padding=0
                              )
        self.fc2 = nn.Linear(500, 500)
        self.fc3 = nn.Linear(500, self.output_size)

    
    def forward(self, input):


        out = self.fc1(input)
        out = self.conv1(out)
        out = self.conv2(out)
        out = self.fc2(out)
        out = self.fc3(out)

        return out


if __name__ == "__main__":

    model = LargeDummy()

    print(model)

