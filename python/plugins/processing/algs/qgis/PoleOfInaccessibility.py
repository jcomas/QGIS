# -*- coding: utf-8 -*-

"""
***************************************************************************
    PoleOfInaccessibility.py
    ------------------------
    Date                 : November 2016
    Copyright            : (C) 2016 by Nyall Dawson
    Email                : nyall dot dawson at gmail dot com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************
"""

__author__ = 'Nyall Dawson'
__date__ = 'November 2016'
__copyright__ = '(C) 2016, Nyall Dawson'

# This will get replaced with a git SHA1 when you do a git archive323

__revision__ = '$Format:%H$'

import os

from qgis.core import QgsWkbTypes, QgsField, NULL, QgsFeatureSink, QgsProcessingUtils

from qgis.PyQt.QtCore import QVariant
from qgis.PyQt.QtGui import QIcon

from processing.algs.qgis.QgisAlgorithm import QgisAlgorithm
from processing.core.GeoAlgorithmExecutionException import GeoAlgorithmExecutionException
from processing.core.parameters import ParameterVector, ParameterNumber
from processing.core.outputs import OutputVector
from processing.tools import dataobjects

pluginPath = os.path.split(os.path.split(os.path.dirname(__file__))[0])[0]


class PoleOfInaccessibility(QgisAlgorithm):

    INPUT_LAYER = 'INPUT_LAYER'
    TOLERANCE = 'TOLERANCE'
    OUTPUT_LAYER = 'OUTPUT_LAYER'

    def icon(self):
        return QIcon(os.path.join(pluginPath, 'images', 'ftools', 'centroids.png'))

    def tags(self):
        return self.tr('furthest,point,distant,extreme,maximum,centroid,center,centre').split(',')

    def group(self):
        return self.tr('Vector geometry tools')

    def __init__(self):
        super().__init__()

    def initAlgorithm(self, config=None):
        self.addParameter(ParameterVector(self.INPUT_LAYER,
                                          self.tr('Input layer'),
                                          [dataobjects.TYPE_VECTOR_POLYGON]))
        self.addParameter(ParameterNumber(self.TOLERANCE,
                                          self.tr('Tolerance (layer units)'), default=1.0, minValue=0.0))
        self.addOutput(OutputVector(self.OUTPUT_LAYER, self.tr('Point'), datatype=[dataobjects.TYPE_VECTOR_POINT]))

    def name(self):
        return 'poleofinaccessibility'

    def displayName(self):
        return self.tr('Pole of Inaccessibility')

    def processAlgorithm(self, parameters, context, feedback):
        layer = QgsProcessingUtils.mapLayerFromString(self.getParameterValue(self.INPUT_LAYER), context)
        tolerance = self.getParameterValue(self.TOLERANCE)

        fields = layer.fields()
        fields.append(QgsField('dist_pole', QVariant.Double))

        writer = self.getOutputFromName(
            self.OUTPUT_LAYER).getVectorWriter(fields, QgsWkbTypes.Point, layer.crs(), context)

        features = QgsProcessingUtils.getFeatures(layer, context)
        total = 100.0 / layer.featureCount() if layer.featureCount() else 0

        for current, input_feature in enumerate(features):
            output_feature = input_feature
            input_geometry = input_feature.geometry()
            if input_geometry:
                output_geometry, distance = input_geometry.poleOfInaccessibility(tolerance)
                if not output_geometry:
                    raise GeoAlgorithmExecutionException(
                        self.tr('Error calculating pole of inaccessibility'))
                attrs = input_feature.attributes()
                attrs.append(distance)
                output_feature.setAttributes(attrs)

                output_feature.setGeometry(output_geometry)
            else:
                attrs = input_feature.attributes()
                attrs.append(NULL)
                output_feature.setAttributes(attrs)

            writer.addFeature(output_feature, QgsFeatureSink.FastInsert)
            feedback.setProgress(int(current * total))

        del writer
