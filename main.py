# -*- coding: utf-8 -*-
"""
Created on Sat Jan 15

@author: Paul Geiger
"""

import sys
import matplotlib.pyplot as plt
import numpy as np

from ctypes import CDLL
from ctypes import c_int
from ctypes import c_double
from ctypes import create_string_buffer
from functools import partial
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QApplication
from PyQt5.QtWidgets import QFileDialog
from PyQt5.QtWidgets import QGridLayout
from PyQt5.QtWidgets import QLabel
from PyQt5.QtWidgets import QLineEdit
from PyQt5.QtWidgets import QMainWindow
from PyQt5.QtWidgets import QPushButton
from PyQt5.QtWidgets import QSizePolicy
from PyQt5.QtWidgets import QVBoxLayout
from PyQt5.QtWidgets import QWidget

ERROR_MSG = 'There was an issue in solving this problem'
NOT_A_NUMBER = 'This is not a valid input'

class MyUI(QMainWindow):        
    """Defines the UI"""
    
    # Define buttons and layout here for simplicity
    myButtons = {
               '7': (0, 0),
               '8': (0, 1),
               '9': (0, 2),
               '/': (0, 3),
               'C': (0, 4),
               '4': (1, 0),
               '5': (1, 1),
               '6': (1, 2),
               '*': (1, 3),
               '(': (1, 4),
               '1': (2, 0),
               '2': (2, 1),
               '3': (2, 2),
               '-': (2, 3),
               ')': (2, 4),
               '0': (3, 0),
               '.': (3, 1),
               '^': (3, 2),
               '+': (3, 3),
               'e': (3, 4),
               'x': (4, 0),
               'sin': (4, 1),
               'cos': (4, 2),
               'tan': (4, 3),
               'EVAL': (4, 4),
              }
    
    def __init__(self):
        """View initializer."""
        super().__init__()
        
        # Set the title and size of the window
        self.setWindowTitle('Roots')
        self.setFixedSize(500, 300)
        
        # Set the general layout and central widget
        self.generalLayout = QVBoxLayout()
        self._centralWidget = QWidget(self)
        self.setCentralWidget(self._centralWidget)
        self._centralWidget.setLayout(self.generalLayout)
        
        # Create the display and the buttons
        self._createDisplay()
        self._createButtons()
        self._createTextboxes()
        
    def _createDisplay(self):
        """Create the display."""
        # Create the display widget
        self.display = QLineEdit()
        
        # Set some display's properties
        self.display.setFixedHeight(40)
        self.display.setAlignment(Qt.AlignRight)
        self.display.setReadOnly(False)
        
        # Add the display to the general layout
        self.generalLayout.addWidget(self.display)
        
    def _createButtons(self):
        """Create the buttons."""
        buttonsLayout = QGridLayout()
        
        # Create the buttons and add them to the grid layout
        self.buttons = {}
        for btnText, pos in self.myButtons.items():
            self.buttons[btnText] = QPushButton(btnText)
            self.buttons[btnText].setFixedSize(60, 30)
            buttonsLayout.addWidget(self.buttons[btnText], pos[0], pos[1])
            
        #  Add it to the layout
        self.generalLayout.addLayout(buttonsLayout) 
        
    def _createTextboxes(self):
        """Create the initial guess, maximum iterations, and goal boxes."""        
        textBoxLayout = QGridLayout()
        
        # Add the editable textboxes
        size = 30
        self.initialGuessBox = self._createStandardTextbox("0.0", size)
        self.maxIterations = self._createStandardTextbox("10000", size)
        self.goalErr = self._createStandardTextbox("1.0E-4", size)
        
        # Add some labels for the editable
        self.initialGuessBoxLabel = self._createStandardLabel("Initial Guess")
        self.maxIterationsLabel = self._createStandardLabel("Maximum Number of Iterations")        
        self.goalErrLabel = self._createStandardLabel("Maximum Absolute Error")
        
        # Add the new items to the grid layout
        textBoxLayout.addWidget(self.initialGuessBoxLabel, 0, 0)
        textBoxLayout.addWidget(self.initialGuessBox, 1, 0)
        
        textBoxLayout.addWidget(self.maxIterationsLabel, 0, 1)
        textBoxLayout.addWidget(self.maxIterations, 1, 1)
        
        textBoxLayout.addWidget(self.goalErrLabel, 0, 2)
        textBoxLayout.addWidget(self.goalErr, 1, 2)
        
        # Add it to the layout
        self.generalLayout.addLayout(textBoxLayout)
        
    def _createFileDialog(self):
        """Create the file dialog boxes."""        
        textBoxLayout = QGridLayout()
        
        # Add the editable textboxes
        size = 30
        self.initialGuessBox = self._createStandardTextbox("0.1", size)
        self.maxIterations = self._createStandardTextbox("10000", size)
        self.goalErr = self._createStandardTextbox("1.0E-4", size)
        
        # Add some labels for the editable
        self.initialGuessBoxLabel = self._createStandardLabel("Initial Guess")
        self.maxIterationsLabel = self._createStandardLabel("Maximum Number of Iterations")        
        self.goalErrLabel = self._createStandardLabel("Maximum Absolute Error")
        
        # Add the new items to the grid layout
        textBoxLayout.addWidget(self.initialGuessBoxLabel, 0, 0)
        textBoxLayout.addWidget(self.initialGuessBox, 1, 0)
        
        textBoxLayout.addWidget(self.maxIterationsLabel, 0, 1)
        textBoxLayout.addWidget(self.maxIterations, 1, 1)
        
        textBoxLayout.addWidget(self.goalErrLabel, 0, 2)
        textBoxLayout.addWidget(self.goalErr, 1, 2)
        
        # Add it to the layout
        self.generalLayout.addLayout(textBoxLayout)
    def _createStandardTextbox(self, initialValue, size):
        textBox = QLineEdit(self)
        textBox.setFixedHeight(size)
        textBox.setAlignment(Qt.AlignRight)
        textBox.setReadOnly(False)
        textBox.setText(initialValue)
        textBox.setFocus()
        return textBox
        
    def _createStandardLabel(self, value):
        label = QLabel(value, self)
        label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        label.setAlignment(Qt.AlignCenter)
        return label
        
    def setDisplayText(self, text):
        """Set display's text."""
        self.display.setText(text)
        self.display.setFocus()

    def displayText(self):
        """Get display's text."""
        return self.display.text()

    def clearDisplay(self):
        """Clear the display."""
        self.setDisplayText('')
    
    def retrieveInputs(self):
        """Retrieve the initial guess, maximum iterations, and goal values. Issue an error if invalid. First value is whether there were errors"""
        hadErrors = False
        initGuess = 0.0
        maxIterations = 0
        goalErr = 0.0
        
        try:
            initGuess = float(self.initialGuessBox.text())
        except ValueError:
            self.initialGuessBox.setText(NOT_A_NUMBER)
            self.initialGuessBox.setFocus()
            hadErrors = True
        
        try:
            maxIterations = int(self.maxIterations.text())
        except ValueError:
            self.maxIterations.setText(NOT_A_NUMBER)
            self.maxIterations.setFocus()
            hadErrors = True
            
        try:
            goalErr = float(self.goalErr.text())
        except ValueError:
            self.goalErr.setText(NOT_A_NUMBER)
            self.goalErr.setFocus()
            hadErrors = True
            
        return (hadErrors, initGuess, maxIterations, goalErr)
        
        
# Create a Controller class to connect the GUI and the model
class MyController:
    """The controller for MyUI."""
    def __init__(self, model, view):
        """Controller initializer."""
        self._evaluate = model
        self._view = view
        self._connectSignals() # Connect signals and slots

    def _calculateResult(self):
        """Evaluate expressions."""
        expr = self._view.displayText() 
        
        (hadErrors, initGuess, maxIterations, goalErr) = self._view.retrieveInputs()
        
        if (not hadErrors):
            result = self._evaluate(expr, initGuess, maxIterations, goalErr) 
            self._view.setDisplayText(result)

    def _buildExpression(self, subExpr):
        """Build expression."""
        if self._view.displayText() == ERROR_MSG:
            self._view.clearDisplay()

        expression = self._view.displayText() + subExpr
        self._view.setDisplayText(expression)

    def _connectSignals(self):
        """Connect signals and slots."""
        for btnText, btn in self._view.buttons.items():
            if btnText not in {'EVAL', 'C'}:
                btn.clicked.connect(partial(self._buildExpression, btnText))

        self._view.buttons['EVAL'].clicked.connect(self._calculateResult)
        self._view.display.returnPressed.connect(self._calculateResult)
        self._view.buttons['C'].clicked.connect(self._view.clearDisplay)


class MyEvaluator:
    """The Evaluator for MyController. Links Controller to DLL."""    
    def __init__(self, dllLocation):
        self.lib = CDLL(dllLocation)
        
    def evaluateExpression(self, expression, initialGuess, maxNumberOfIterations, goalErr):
        """Evaluate an expression."""
        
         # Results array is created on the python side, so memory management is automatic
        results = (c_double * maxNumberOfIterations)()
        cInitGuess = c_double(initialGuess)
        cMaxNum = c_int(maxNumberOfIterations)
        cGoalErr = c_double(goalErr)
        cExpr = create_string_buffer(expression.encode())
        numResults = self.lib.SolveForRoot(cExpr.value, len(expression), cInitGuess, cMaxNum, cGoalErr, results)
        arrResults = np.ctypeslib.as_array(results)
        
        plt.plot(arrResults[0:numResults-1])
        plt.xlabel('Iteration Number')
        plt.ylabel('Solution Estimate')
        plt.show()
        return str(results[numResults-1]);

def main():
    app = QApplication(sys.argv)
    
    ui = MyUI()
    ui.show()
    
    # Create instances of the model and the controller
    evaluator = MyEvaluator(r'RootFinder.dll')
    model = evaluator.evaluateExpression
    MyController(model=model, view=ui)
    
    sys.exit(app.exec_())
    
if __name__ == '__main__':
    main()