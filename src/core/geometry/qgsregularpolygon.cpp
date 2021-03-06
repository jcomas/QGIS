/***************************************************************************
                         qgsregularpolygon.cpp
                         --------------
    begin                : May 2017
    copyright            : (C) 2017 by Loîc Bartoletti
    email                : lbartoletti at tuxfamily dot org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsregularpolygon.h"
#include "qgsgeometryutils.h"

#include <memory>

QgsRegularPolygon::QgsRegularPolygon()
  : mCenter( QgsPoint() )
  , mFirstVertex( QgsPoint() )
  , mNumberSides( 0 )
  , mRadius( 0.0 )
{

}


QgsRegularPolygon::QgsRegularPolygon( const QgsPoint &center, const double radius, const double azimuth, const int numSides, const ConstructionOption circle )
  : mCenter( center )
  , mFirstVertex( QgsPoint() )
  , mNumberSides( 0 )
  , mRadius( 0.0 )
{
  // TODO: inclination

  if ( numSides >= 3 )
  {
    mNumberSides = numSides;

    switch ( circle )
    {
      case InscribedCircle:
      {
        mRadius = qAbs( radius );
        mFirstVertex = mCenter.project( mRadius, azimuth );
        break;
      }
      case CircumscribedCircle:
      {
        mRadius = apothemToRadius( qAbs( radius ), numSides );
        mFirstVertex = mCenter.project( mRadius, azimuth - centralAngle( numSides ) / 2 );
        break;
      }
      default:
        break;
    }

  }

}

QgsRegularPolygon::QgsRegularPolygon( const QgsPoint &center, const QgsPoint &pt1, const int numSides, const ConstructionOption circle )
  : mCenter( center )
  , mFirstVertex( QgsPoint() )
  , mNumberSides( 0 )
  , mRadius( 0.0 )
{
  if ( numSides >= 3 )
  {
    mNumberSides = numSides;

    switch ( circle )
    {
      case InscribedCircle:
      {
        mFirstVertex = pt1;
        mRadius = center.distance( pt1 );
        break;
      }
      case CircumscribedCircle:
      {
        mRadius = apothemToRadius( center.distance( pt1 ), numSides );
        double azimuth = center.azimuth( pt1 );
        // TODO: inclination
        mFirstVertex = mCenter.project( mRadius, azimuth - centralAngle( numSides ) / 2 );
        break;
      }
      default:
        break;
    }

  }

}

QgsRegularPolygon::QgsRegularPolygon( const QgsPoint &pt1, const QgsPoint &pt2, const int numSides )
  : mCenter( QgsPoint() )
  , mFirstVertex( QgsPoint() )
  , mNumberSides( 0 )
  , mRadius( 0.0 )
{
  if ( numSides >= 3 )
  {
    mNumberSides = numSides;

    double azimuth = pt1.azimuth( pt2 );
    QgsPoint pm = QgsGeometryUtils::midpoint( pt1, pt2 );
    double length = pt1.distance( pm );

    double angle = ( 180 - ( 360 / numSides ) ) / 2.0;
    double hypothenuse = length / cos( angle * M_PI / 180 );
    // TODO: inclination

    mCenter = pt1.project( hypothenuse, azimuth + angle );
    mFirstVertex = pt1;
    mRadius = qAbs( hypothenuse );
  }
}

bool QgsRegularPolygon::operator ==( const QgsRegularPolygon &rp ) const
{
  return ( ( mCenter == rp.mCenter ) &&
           ( mFirstVertex == rp.mFirstVertex ) &&
           ( mNumberSides == rp.mNumberSides )
         );
}

bool QgsRegularPolygon::operator !=( const QgsRegularPolygon &rp ) const
{
  return !operator==( rp );
}

bool QgsRegularPolygon::isEmpty() const
{
  return ( ( mNumberSides < 3 ) ||
           ( mCenter == mFirstVertex )
         );
}

void QgsRegularPolygon::setCenter( const QgsPoint &center )
{
  double azimuth = mCenter.azimuth( mFirstVertex );
  // TODO: double inclination = mCenter.inclination(mFirstVertex);
  mCenter = center;
  mFirstVertex = center.project( mRadius, azimuth );
}

void QgsRegularPolygon::setRadius( const double radius )
{
  mRadius = qAbs( radius );
  double azimuth = mCenter.azimuth( mFirstVertex );
  // TODO: double inclination = mCenter.inclination(mFirstVertex);
  mFirstVertex = mCenter.project( mRadius, azimuth );
}

void QgsRegularPolygon::setFirstVertex( const QgsPoint &firstVertex )
{
  double azimuth = mCenter.azimuth( mFirstVertex );
  // TODO: double inclination = mCenter.inclination(firstVertex);
  mFirstVertex = firstVertex;
  mCenter = mFirstVertex.project( mRadius, azimuth );
}

void QgsRegularPolygon::setNumberSides( const int numSides )
{
  if ( numSides >= 3 )
  {
    mNumberSides = numSides;
  }
}

QgsPointSequence QgsRegularPolygon::points( ) const
{
  QgsPointSequence pts;
  if ( isEmpty() )
  {
    return pts;
  }

  double azimuth =  mCenter.azimuth( mFirstVertex );
  double azimuth_add = centralAngle();
  // TODO: inclination

  unsigned int n = 1;
  while ( n <= mNumberSides )
  {
    pts.push_back( mCenter.project( mRadius, azimuth ) );
    azimuth += azimuth_add;
    if ( ( azimuth_add > 0 ) && ( azimuth > 180.0 ) )
    {
      azimuth -= 360.0;
    }

    n++;
  }

  return pts;
}

QgsPolygonV2 *QgsRegularPolygon::toPolygon() const
{
  std::unique_ptr<QgsPolygonV2> polygon( new QgsPolygonV2() );
  if ( isEmpty() )
  {
    return polygon.release();
  }

  polygon->setExteriorRing( toLineString( ) );

  return polygon.release();
}

QgsLineString *QgsRegularPolygon::toLineString() const
{
  std::unique_ptr<QgsLineString> ext( new QgsLineString() );
  if ( isEmpty() )
  {
    return ext.release();
  }

  QgsPointSequence pts;
  pts = points( );

  ext->setPoints( pts );

  return ext.release();
}

QgsTriangle QgsRegularPolygon::toTriangle() const
{
  if ( isEmpty() || ( mNumberSides != 3 ) )
  {
    return QgsTriangle();
  }

  QgsPointSequence pts;
  pts = points( );

  return QgsTriangle( pts.at( 0 ), pts.at( 1 ), pts.at( 2 ) );
}

QList<QgsTriangle> QgsRegularPolygon::triangulate() const
{
  QList<QgsTriangle> l_tri;
  if ( isEmpty() )
  {
    return l_tri;
  }

  QgsPointSequence pts;
  pts = points( );

  unsigned int n = 0;
  while ( n < mNumberSides - 1 )
  {
    l_tri.append( QgsTriangle( pts.at( n ), pts.at( n + 1 ), mCenter ) );
    n++;
  }
  l_tri.append( QgsTriangle( pts.at( n ), pts.at( 0 ), mCenter ) );

  return l_tri;
}

QgsCircle QgsRegularPolygon::inscribedCircle() const
{
  // TODO: inclined circle
  return QgsCircle( mCenter, apothem() );
}

QgsCircle QgsRegularPolygon::circumscribedCircle() const
{
  // TODO: inclined circle
  return QgsCircle( mCenter, mRadius );
}

QString QgsRegularPolygon::toString( int pointPrecision, int radiusPrecision, int anglePrecision ) const
{
  QString rep;
  if ( isEmpty() )
    rep = QStringLiteral( "Empty" );
  else
    rep = QStringLiteral( "RegularPolygon (Center: %1, First Vertex: %2, Radius: %3, Azimuth: %4)" )
          .arg( mCenter.asWkt( pointPrecision ), 0, 's' )
          .arg( mFirstVertex.asWkt( pointPrecision ), 0, 's' )
          .arg( qgsDoubleToString( mRadius, radiusPrecision ), 0, 'f' )
          .arg( qgsDoubleToString( mCenter.azimuth( mFirstVertex ), anglePrecision ), 0, 'f' );
  // TODO: inclination
  // .arg( qgsDoubleToString( mCenter.inclination(mFirstVertex), anglePrecision ), 0, 'f' );

  return rep;
}

double QgsRegularPolygon::area() const
{
  if ( isEmpty() )
  {
    return 0.0;
  }

  return ( mRadius * mRadius * mNumberSides * sin( centralAngle() * M_PI / 180.0 ) ) / 2 ;
}

double QgsRegularPolygon::perimeter() const
{
  if ( isEmpty() )
  {
    return 0.0;
  }

  return length() * mNumberSides;
}

double QgsRegularPolygon::length() const
{
  if ( isEmpty() )
  {
    return 0.0;
  }

  return mRadius * 2 * sin( M_PI / mNumberSides );
}

double QgsRegularPolygon::apothemToRadius( const double apothem, const unsigned int numSides ) const
{
  return apothem / cos( M_PI / numSides );
}

double QgsRegularPolygon::interiorAngle( const unsigned int nbSides ) const
{
  return ( nbSides - 2 ) * 180 / nbSides;
}

double QgsRegularPolygon::centralAngle( const unsigned int nbSides ) const
{
  return 360.0 / nbSides;
}

double QgsRegularPolygon::interiorAngle() const
{
  return interiorAngle( mNumberSides );
}

double QgsRegularPolygon::centralAngle() const
{
  return centralAngle( mNumberSides );
}
