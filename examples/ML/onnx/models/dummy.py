import torch
from torch import nn


class Dummy(nn.Module):

    def __init__(self):
        super().__init__()

        self.input_size = 1
        self.output_size = 1

        self.fc1 = nn.Linear(self.input_size, 500)
        self.fc2 = nn.Linear(500, 500)
        self.fc3 = nn.Linear(500, self.output_size)

    
    def forward(self, input):


        out = self.fc1(input)
        out = self.fc2(out)
        out = self.fc3(out)

        return out


if __name__ == "__main__":

    model = Dummy()

    print(model)

