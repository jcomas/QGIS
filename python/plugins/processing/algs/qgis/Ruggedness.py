# -*- coding: utf-8 -*-

"""
***************************************************************************
    Ruggedness.py
    ---------------------
    Date                 : October 2016
    Copyright            : (C) 2016 by Alexander Bruy
    Email                : alexander dot bruy at gmail dot com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************
"""

__author__ = 'Alexander Bruy'
__date__ = 'October 2016'
__copyright__ = '(C) 2016, Alexander Bruy'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import os

from qgis.PyQt.QtGui import QIcon

from qgis.analysis import QgsRuggednessFilter

from processing.algs.qgis.QgisAlgorithm import QgisAlgorithm
from processing.core.parameters import ParameterRaster
from processing.core.parameters import ParameterNumber
from processing.core.outputs import OutputRaster
from processing.tools import raster

pluginPath = os.path.split(os.path.split(os.path.dirname(__file__))[0])[0]


class Ruggedness(QgisAlgorithm):

    INPUT_LAYER = 'INPUT_LAYER'
    Z_FACTOR = 'Z_FACTOR'
    OUTPUT_LAYER = 'OUTPUT_LAYER'

    def icon(self):
        return QIcon(os.path.join(pluginPath, 'images', 'dem.png'))

    def group(self):
        return self.tr('Raster terrain analysis')

    def __init__(self):
        super().__init__()

    def initAlgorithm(self, config=None):
        self.addParameter(ParameterRaster(self.INPUT_LAYER,
                                          self.tr('Elevation layer')))
        self.addParameter(ParameterNumber(self.Z_FACTOR,
                                          self.tr('Z factor'), 1.0, 999999.99, 1.0))
        self.addOutput(OutputRaster(self.OUTPUT_LAYER,
                                    self.tr('Ruggedness index')))

    def name(self):
        return 'ruggednessindex'

    def displayName(self):
        return self.tr('Ruggedness index')

    def processAlgorithm(self, parameters, context, feedback):
        inputFile = self.getParameterValue(self.INPUT_LAYER)
        zFactor = self.getParameterValue(self.Z_FACTOR)
        outputFile = self.getOutputValue(self.OUTPUT_LAYER)

        outputFormat = raster.formatShortNameFromFileName(outputFile)

        ruggedness = QgsRuggednessFilter(inputFile, outputFile, outputFormat)
        ruggedness.setZFactor(zFactor)
        ruggedness.processRaster(None)
