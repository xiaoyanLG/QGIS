
/***************************************************************************
     testqgsogcutils.cpp
     --------------------------------------
    Date                 : March 2013
    Copyright            : (C) 2013 Martin Dobias
    Email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgstest.h"
#include <memory>

//qgis includes...
#include <qgsgeometry.h>
#include <qgsogcutils.h>
#include "qgsapplication.h"

/**
 * \ingroup UnitTests
 * This is a unit test for OGC utilities
 */
class TestQgsOgcUtils : public QObject
{
    Q_OBJECT
  private slots:

    void initTestCase()
    {
      // Needed on Qt 5 so that the serialization of XML is consistent among all executions
#if QT_VERSION >= 0x50600
      qSetGlobalQHashSeed( 0 );
#else
      extern Q_CORE_EXPORT QBasicAtomicInt qt_qhash_seed;
      qt_qhash_seed.store( 0 );
#endif

      //
      // Runs once before any tests are run
      //
      // init QGIS's paths - true means that all path will be inited from prefix
      QgsApplication::init();
      QgsApplication::initQgis();
    }

    void cleanupTestCase()
    {
      QgsApplication::exitQgis();
    }

    void testGeometryFromGML();
    void testGeometryToGML();

    void testExpressionFromOgcFilter();
    void testExpressionFromOgcFilter_data();

    void testExpressionToOgcFilter();
    void testExpressionToOgcFilter_data();

    void testExpressionToOgcFilterWFS11();
    void testExpressionToOgcFilterWFS11_data();

    void testExpressionToOgcFilterWFS20();
    void testExpressionToOgcFilterWFS20_data();

    void testSQLStatementToOgcFilter();
    void testSQLStatementToOgcFilter_data();
};


void TestQgsOgcUtils::testGeometryFromGML()
{
  // Test GML2
  QgsGeometry geom( QgsOgcUtils::geometryFromGML( QStringLiteral( "<Point><coordinates>123,456</coordinates></Point>" ) ) );
  QVERIFY( geom );
  QVERIFY( geom.wkbType() == QgsWkbTypes::Point );
  QVERIFY( geom.asPoint() == QgsPointXY( 123, 456 ) );

  QgsGeometry geomBox( QgsOgcUtils::geometryFromGML( QStringLiteral( "<gml:Box srsName=\"foo\"><gml:coordinates>135.2239,34.4879 135.8578,34.8471</gml:coordinates></gml:Box>" ) ) );
  QVERIFY( geomBox );
  QVERIFY( geomBox.wkbType() == QgsWkbTypes::Polygon );


  // Test GML3
  geom = QgsOgcUtils::geometryFromGML( QStringLiteral( "<Point><pos>123 456</pos></Point>" ) );
  QVERIFY( geom );
  QVERIFY( geom.wkbType() == QgsWkbTypes::Point );
  QVERIFY( geom.asPoint() == QgsPointXY( 123, 456 ) );

  geomBox = QgsOgcUtils::geometryFromGML( QStringLiteral( "<gml:Envelope srsName=\"foo\"><gml:lowerCorner>135.2239 34.4879</gml:lowerCorner><gml:upperCorner>135.8578 34.8471</gml:upperCorner></gml:Envelope>" ) );
  QVERIFY( geomBox );
  QVERIFY( geomBox.wkbType() == QgsWkbTypes::Polygon );
}

static bool compareElements( QDomElement &element1, QDomElement &element2 )
{
  QString tag1 = element1.tagName();
  tag1.replace( QRegExp( ".*:" ), QLatin1String( "" ) );
  QString tag2 = element2.tagName();
  tag2.replace( QRegExp( ".*:" ), QLatin1String( "" ) );
  if ( tag1 != tag2 )
  {
    qDebug( "Different tag names: %s, %s", tag1.toAscii().data(), tag2.toAscii().data() );
    return false ;
  }

  if ( element1.hasAttributes() != element2.hasAttributes() )
  {
    qDebug( "Different hasAttributes: %s, %s", tag1.toAscii().data(), tag2.toAscii().data() );
    return false;
  }

  if ( element1.hasAttributes() )
  {
    QDomNamedNodeMap attrs1 = element1.attributes();
    QDomNamedNodeMap attrs2 = element2.attributes();

    if ( attrs1.size() != attrs2.size() )
    {
      qDebug( "Different attributes size: %s, %s", tag1.toAscii().data(), tag2.toAscii().data() );
      return false;
    }

    for ( int i = 0 ; i < attrs1.size() ; ++i )
    {
      QDomNode node1 = attrs1.item( i );
      QDomAttr attr1 = node1.toAttr();

      if ( !element2.hasAttribute( attr1.name() ) )
      {
        qDebug( "Element2 has not attribute: %s, %s, %s", tag1.toAscii().data(), tag2.toAscii().data(), attr1.name().toAscii().data() );
        return false;
      }

      if ( element2.attribute( attr1.name() ) != attr1.value() )
      {
        qDebug( "Element2 attribute has not the same value: %s, %s, %s", tag1.toAscii().data(), tag2.toAscii().data(), attr1.name().toAscii().data() );
        return false;
      }
    }
  }

  if ( element1.hasChildNodes() != element2.hasChildNodes() )
  {
    qDebug( "Different childNodes: %s, %s", tag1.toAscii().data(), tag2.toAscii().data() );
    return false;
  }

  if ( element1.hasChildNodes() )
  {
    QDomNodeList nodes1 = element1.childNodes();
    QDomNodeList nodes2 = element2.childNodes();

    if ( nodes1.size() != nodes2.size() )
    {
      qDebug( "Different childNodes size: %s, %s", tag1.toAscii().data(), tag2.toAscii().data() );
      return false;
    }

    for ( int i = 0 ; i < nodes1.size() ; ++i )
    {
      QDomNode node1 = nodes1.at( i );
      QDomNode node2 = nodes2.at( i );
      if ( node1.isElement() && node2.isElement() )
      {
        QDomElement elt1 = node1.toElement();
        QDomElement elt2 = node2.toElement();

        if ( !compareElements( elt1, elt2 ) )
          return false;
      }
      else if ( node1.isText() && node2.isText() )
      {
        QDomText txt1 = node1.toText();
        QDomText txt2 = node2.toText();

        if ( txt1.data() != txt2.data() )
        {
          qDebug( "Different text data: %s %s", tag1.toAscii().data(), txt1.data().toAscii().data() );
          qDebug( "Different text data: %s %s", tag2.toAscii().data(), txt2.data().toAscii().data() );
          return false;
        }
      }
    }
  }

  if ( element1.text() != element2.text() )
  {
    qDebug( "Different text: %s %s", tag1.toAscii().data(), element1.text().toAscii().data() );
    qDebug( "Different text: %s %s", tag2.toAscii().data(), element2.text().toAscii().data() );
    return false;
  }

  return true;
}
static QDomElement comparableElement( const QString &xmlText )
{
  QDomDocument doc;
  if ( !doc.setContent( xmlText ) )
    return QDomElement();
  return doc.documentElement();
}


void TestQgsOgcUtils::testGeometryToGML()
{
  QDomDocument doc;
  QgsGeometry geomPoint( QgsGeometry::fromPoint( QgsPointXY( 111, 222 ) ) );
  QgsGeometry geomLine( QgsGeometry::fromWkt( QStringLiteral( "LINESTRING(111 222, 222 222)" ) ) );

  // Elements to compare
  QDomElement xmlElem;
  QDomElement ogcElem;

  // Test GML2
  QDomElement elemInvalid = QgsOgcUtils::geometryToGML( QgsGeometry(), doc );
  QVERIFY( elemInvalid.isNull() );

  QDomElement elemPoint = QgsOgcUtils::geometryToGML( geomPoint, doc );
  QVERIFY( !elemPoint.isNull() );

  doc.appendChild( elemPoint );
  xmlElem = comparableElement( QStringLiteral( "<gml:Point><gml:coordinates ts=\" \" cs=\",\">111,222</gml:coordinates></gml:Point>" ) );
  ogcElem = comparableElement( doc.toString( -1 ) );
  QVERIFY( compareElements( xmlElem, ogcElem ) );
  doc.removeChild( elemPoint );

  QDomElement elemLine = QgsOgcUtils::geometryToGML( geomLine, doc );
  QVERIFY( !elemLine.isNull() );

  doc.appendChild( elemLine );
  xmlElem = comparableElement( QStringLiteral( "<gml:LineString><gml:coordinates ts=\" \" cs=\",\">111,222 222,222</gml:coordinates></gml:LineString>" ) );
  ogcElem = comparableElement( doc.toString( -1 ) );
  QVERIFY( compareElements( xmlElem, ogcElem ) );
  doc.removeChild( elemLine );

  // Test GML3
  elemInvalid = QgsOgcUtils::geometryToGML( QgsGeometry(), doc, QStringLiteral( "GML3" ) );
  QVERIFY( elemInvalid.isNull() );

  elemPoint = QgsOgcUtils::geometryToGML( geomPoint, doc, QStringLiteral( "GML3" ) );
  QVERIFY( !elemPoint.isNull() );

  doc.appendChild( elemPoint );
  xmlElem = comparableElement( QStringLiteral( "<gml:Point><gml:pos srsDimension=\"2\">111 222</gml:pos></gml:Point>" ) );
  ogcElem = comparableElement( doc.toString( -1 ) );
  QVERIFY( compareElements( xmlElem, ogcElem ) );
  doc.removeChild( elemPoint );

  elemLine = QgsOgcUtils::geometryToGML( geomLine, doc, QStringLiteral( "GML3" ) );
  QVERIFY( !elemLine.isNull() );

  doc.appendChild( elemLine );
  xmlElem = comparableElement( QStringLiteral( "<gml:LineString><gml:posList srsDimension=\"2\">111 222 222 222</gml:posList></gml:LineString>" ) );
  ogcElem = comparableElement( doc.toString( -1 ) );
  QVERIFY( compareElements( xmlElem, ogcElem ) );
  doc.removeChild( elemLine );
}


void TestQgsOgcUtils::testExpressionFromOgcFilter_data()
{
  QTest::addColumn<QString>( "xmlText" );
  QTest::addColumn<QString>( "dumpText" );

  QTest::newRow( "=" ) << QString(
                         "<Filter><PropertyIsEqualTo>"
                         "<PropertyName>NAME</PropertyName>"
                         "<Literal>New York</Literal>"
                         "</PropertyIsEqualTo></Filter>" )
                       << QStringLiteral( "NAME = 'New York'" );

  QTest::newRow( ">" ) << QString(
                         "<Filter><PropertyIsGreaterThan>"
                         "<PropertyName>COUNT</PropertyName>"
                         "<Literal>3</Literal>"
                         "</PropertyIsGreaterThan></Filter>" )
                       << QStringLiteral( "COUNT > 3" );

  QTest::newRow( "AND" ) << QString(
                           "<ogc:Filter>"
                           "<ogc:And>"
                           "<ogc:PropertyIsGreaterThanOrEqualTo>"
                           "<ogc:PropertyName>pop</ogc:PropertyName>"
                           "<ogc:Literal>50000</ogc:Literal>"
                           "</ogc:PropertyIsGreaterThanOrEqualTo>"
                           "<ogc:PropertyIsLessThan>"
                           "<ogc:PropertyName>pop</ogc:PropertyName>"
                           "<ogc:Literal>100000</ogc:Literal>"
                           "</ogc:PropertyIsLessThan>"
                           "</ogc:And>"
                           "</ogc:Filter>" )
                         << QStringLiteral( "pop >= 50000 AND pop < 100000" );

  // TODO: should work also without <Literal> tags in Lower/Upper-Boundary tags?
  QTest::newRow( "between" ) << QString(
                               "<Filter>"
                               "<PropertyIsBetween><PropertyName>POPULATION</PropertyName>"
                               "<LowerBoundary><Literal>100</Literal></LowerBoundary>"
                               "<UpperBoundary><Literal>200</Literal></UpperBoundary></PropertyIsBetween>"
                               "</Filter>" )
                             << QStringLiteral( "POPULATION >= 100 AND POPULATION <= 200" );

  // handle different wildcards, single chars, escape chars
  QTest::newRow( "like" ) << QString(
                            "<Filter>"
                            "<PropertyIsLike wildCard=\"%\" singleChar=\"_\" escape=\"\\\">"
                            "<PropertyName>NAME</PropertyName><Literal>*QGIS*</Literal></PropertyIsLike>"
                            "</Filter>" )
                          << QStringLiteral( "NAME LIKE '*QGIS*'" );
  QTest::newRow( "ilike" ) << QString(
                             "<Filter>"
                             "<PropertyIsLike matchCase=\"false\" wildCard=\"%\" singleChar=\"_\" escape=\"\\\">"
                             "<PropertyName>NAME</PropertyName><Literal>*QGIS*</Literal></PropertyIsLike>"
                             "</Filter>" )
                           << QStringLiteral( "NAME ILIKE '*QGIS*'" );

  // different wildCards
  QTest::newRow( "like wildCard" ) << QString(
                                     "<Filter>"
                                     "<PropertyIsLike wildCard='*' singleChar='.' escape=\"\\\">"
                                     "<PropertyName>NAME</PropertyName><Literal>*%QGIS*\\*</Literal></PropertyIsLike>"
                                     "</Filter>" )
                                   << QStringLiteral( "NAME LIKE '%\\\\%QGIS%*'" );
  // different single chars
  QTest::newRow( "like single char" ) << QString(
                                        "<Filter>"
                                        "<PropertyIsLike wildCard='*' singleChar='.' escape=\"\\\">"
                                        "<PropertyName>NAME</PropertyName><Literal>._QGIS.\\.</Literal></PropertyIsLike>"
                                        "</Filter>" )
                                      << QStringLiteral( "NAME LIKE '_\\\\_QGIS_.'" );
  // different single chars
  QTest::newRow( "like escape char" ) << QString(
                                        "<Filter>"
                                        "<PropertyIsLike wildCard=\"*\" singleChar=\".\" escape=\"!\">"
                                        "<PropertyName>NAME</PropertyName><Literal>_QGIS.!.!!%QGIS*!*</Literal></PropertyIsLike>"
                                        "</Filter>" )
                                      << QStringLiteral( "NAME LIKE '\\\\_QGIS_.!\\\\%QGIS%*'" );

  QTest::newRow( "is null" ) << QString(
                               "<Filter>"
                               "<ogc:PropertyIsNull>"
                               "<ogc:PropertyName>FIRST_NAME</ogc:PropertyName>"
                               "</ogc:PropertyIsNull>"
                               "</Filter>" )
                             << QStringLiteral( "FIRST_NAME IS NULL" );

  QTest::newRow( "bbox with GML2 Box" ) << QString(
                                          "<Filter>"
                                          "<BBOX><PropertyName>Name>NAME</PropertyName><gml:Box srsName='foo'>"
                                          "<gml:coordinates>135.2239,34.4879 135.8578,34.8471</gml:coordinates></gml:Box></BBOX>"
                                          "</Filter>" )
                                        << QStringLiteral( "intersects_bbox($geometry, geom_from_gml('<Box srsName=\"foo\"><coordinates>135.2239,34.4879 135.8578,34.8471</coordinates></Box>'))" );

  QTest::newRow( "Intersects" ) << QString(
                                  "<Filter>"
                                  "<Intersects>"
                                  "<PropertyName>GEOMETRY</PropertyName>"
                                  "<gml:Point>"
                                  "<gml:coordinates>123,456</gml:coordinates>"
                                  "</gml:Point>"
                                  "</Intersects>"
                                  "</Filter>" )
                                << QStringLiteral( "intersects($geometry, geom_from_gml('<Point><coordinates>123,456</coordinates></Point>'))" );
}

void TestQgsOgcUtils::testExpressionFromOgcFilter()
{
  QFETCH( QString, xmlText );
  QFETCH( QString, dumpText );

  QDomDocument doc;
  QVERIFY( doc.setContent( xmlText, true ) );
  QDomElement rootElem = doc.documentElement();

  std::shared_ptr<QgsExpression> expr( QgsOgcUtils::expressionFromOgcFilter( rootElem ) );
  QVERIFY( expr.get() );

  qDebug( "OGC XML  : %s", xmlText.toAscii().data() );
  qDebug( "EXPR-DUMP: %s", expr->expression().toAscii().data() );

  if ( expr->hasParserError() )
    qDebug( "ERROR: %s ", expr->parserErrorString().toAscii().data() );
  QVERIFY( !expr->hasParserError() );

  QCOMPARE( dumpText, expr->expression() );
}

void TestQgsOgcUtils::testExpressionToOgcFilter()
{
  QFETCH( QString, exprText );
  QFETCH( QString, xmlText );

  QgsExpression exp( exprText );
  QVERIFY( !exp.hasParserError() );

  QString errorMsg;
  QDomDocument doc;
  QDomElement filterElem = QgsOgcUtils::expressionToOgcFilter( exp, doc, &errorMsg );

  if ( !errorMsg.isEmpty() )
    qDebug( "ERROR: %s", errorMsg.toAscii().data() );

  QVERIFY( !filterElem.isNull() );

  doc.appendChild( filterElem );

  qDebug( "EXPR: %s", exp.expression().toAscii().data() );
  qDebug( "OGC : %s", doc.toString( -1 ).toAscii().data() );


  QDomElement xmlElem = comparableElement( xmlText );
  QDomElement ogcElem = comparableElement( doc.toString( -1 ) );
  QVERIFY( compareElements( xmlElem, ogcElem ) );
}

void TestQgsOgcUtils::testExpressionToOgcFilter_data()
{
  QTest::addColumn<QString>( "exprText" );
  QTest::addColumn<QString>( "xmlText" );

  QTest::newRow( "=" ) << QStringLiteral( "NAME = 'New York'" ) << QString(
                         "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                         "<ogc:PropertyIsEqualTo>"
                         "<ogc:PropertyName>NAME</ogc:PropertyName>"
                         "<ogc:Literal>New York</ogc:Literal>"
                         "</ogc:PropertyIsEqualTo></ogc:Filter>" );

  QTest::newRow( ">" ) << QStringLiteral( "\"COUNT\" > 3" ) << QString(
                         "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                         "<ogc:PropertyIsGreaterThan>"
                         "<ogc:PropertyName>COUNT</ogc:PropertyName>"
                         "<ogc:Literal>3</ogc:Literal>"
                         "</ogc:PropertyIsGreaterThan></ogc:Filter>" );

  QTest::newRow( "and+or" ) << QStringLiteral( "(FIELD1 = 10 OR FIELD1 = 20) AND STATUS = 'VALID'" ) << QString(
                              "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                              "<ogc:And>"
                              "<ogc:Or>"
                              "<ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyName>FIELD1</ogc:PropertyName>"
                              "<ogc:Literal>10</ogc:Literal>"
                              "</ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyName>FIELD1</ogc:PropertyName>"
                              "<ogc:Literal>20</ogc:Literal>"
                              "</ogc:PropertyIsEqualTo>"
                              "</ogc:Or>"
                              "<ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyName>STATUS</ogc:PropertyName>"
                              "<ogc:Literal>VALID</ogc:Literal>"
                              "</ogc:PropertyIsEqualTo>"
                              "</ogc:And>"
                              "</ogc:Filter>" );

  QTest::newRow( "like" ) << QStringLiteral( "NAME LIKE '*QGIS*'" ) << QString(
                            "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                            "<ogc:PropertyIsLike singleChar=\"_\" escape=\"\\\" wildCard=\"%\">"
                            "<ogc:PropertyName>NAME</ogc:PropertyName>"
                            "<ogc:Literal>*QGIS*</ogc:Literal>"
                            "</ogc:PropertyIsLike>"
                            "</ogc:Filter>" );

  QTest::newRow( "ilike" ) << QStringLiteral( "NAME ILIKE '*QGIS*'" ) << QString(
                             "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                             "<ogc:PropertyIsLike matchCase=\"false\" singleChar=\"_\" escape=\"\\\" wildCard=\"%\">"
                             "<ogc:PropertyName>NAME</ogc:PropertyName>"
                             "<ogc:Literal>*QGIS*</ogc:Literal>"
                             "</ogc:PropertyIsLike>"
                             "</ogc:Filter>" );

  QTest::newRow( "is null" ) << QStringLiteral( "A IS NULL" ) << QString(
                               "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                               "<ogc:PropertyIsNull>"
                               "<ogc:PropertyName>A</ogc:PropertyName>"
                               "</ogc:PropertyIsNull>"
                               "</ogc:Filter>" );

  QTest::newRow( "is not null" ) << QStringLiteral( "A IS NOT NULL" ) << QString(
                                   "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                                   "<ogc:Not>"
                                   "<ogc:PropertyIsNull>"
                                   "<ogc:PropertyName>A</ogc:PropertyName>"
                                   "</ogc:PropertyIsNull>"
                                   "</ogc:Not>"
                                   "</ogc:Filter>" );

  QTest::newRow( "in" ) << QStringLiteral( "A IN (10,20,30)" ) << QString(
                          "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                          "<ogc:Or>"
                          "<ogc:PropertyIsEqualTo>"
                          "<ogc:PropertyName>A</ogc:PropertyName>"
                          "<ogc:Literal>10</ogc:Literal>"
                          "</ogc:PropertyIsEqualTo>"
                          "<ogc:PropertyIsEqualTo>"
                          "<ogc:PropertyName>A</ogc:PropertyName>"
                          "<ogc:Literal>20</ogc:Literal>"
                          "</ogc:PropertyIsEqualTo>"
                          "<ogc:PropertyIsEqualTo>"
                          "<ogc:PropertyName>A</ogc:PropertyName>"
                          "<ogc:Literal>30</ogc:Literal>"
                          "</ogc:PropertyIsEqualTo>"
                          "</ogc:Or>"
                          "</ogc:Filter>" );

  QTest::newRow( "not in" ) << QStringLiteral( "A NOT IN (10,20,30)" ) << QString(
                              "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                              "<ogc:Not>"
                              "<ogc:Or>"
                              "<ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyName>A</ogc:PropertyName>"
                              "<ogc:Literal>10</ogc:Literal>"
                              "</ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyName>A</ogc:PropertyName>"
                              "<ogc:Literal>20</ogc:Literal>"
                              "</ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyName>A</ogc:PropertyName>"
                              "<ogc:Literal>30</ogc:Literal>"
                              "</ogc:PropertyIsEqualTo>"
                              "</ogc:Or>"
                              "</ogc:Not>"
                              "</ogc:Filter>" );

  QTest::newRow( "intersects_bbox" ) << QStringLiteral( "intersects_bbox($geometry, geomFromWKT('POINT (5 6)'))" ) << QString(
                                       "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\" xmlns:gml=\"http://www.opengis.net/gml\">"
                                       "<ogc:BBOX>"
                                       "<ogc:PropertyName>geometry</ogc:PropertyName>"
                                       "<gml:Box><gml:coordinates ts=\" \" cs=\",\">5,6 5,6</gml:coordinates></gml:Box>"
                                       "</ogc:BBOX>"
                                       "</ogc:Filter>" );

  QTest::newRow( "intersects + wkt" ) << QStringLiteral( "intersects($geometry, geomFromWKT('POINT (5 6)'))" ) << QString(
                                        "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\" xmlns:gml=\"http://www.opengis.net/gml\">"
                                        "<ogc:Intersects>"
                                        "<ogc:PropertyName>geometry</ogc:PropertyName>"
                                        "<gml:Point><gml:coordinates ts=\" \" cs=\",\">5,6</gml:coordinates></gml:Point>"
                                        "</ogc:Intersects>"
                                        "</ogc:Filter>" );

  QTest::newRow( "contains + gml" ) << QStringLiteral( "contains($geometry, geomFromGML('<Point><coordinates cs=\",\" ts=\" \">5,6</coordinates></Point>'))" ) << QString(
                                      "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\" xmlns:gml=\"http://www.opengis.net/gml\">"
                                      "<ogc:Contains>"
                                      "<ogc:PropertyName>geometry</ogc:PropertyName>"
                                      "<Point><coordinates ts=\" \" cs=\",\">5,6</coordinates></Point>"
                                      "</ogc:Contains>"
                                      "</ogc:Filter>" );
}

void TestQgsOgcUtils::testExpressionToOgcFilterWFS11()
{
  QFETCH( QString, exprText );
  QFETCH( QString, srsName );
  QFETCH( QString, xmlText );

  QgsExpression exp( exprText );
  QVERIFY( !exp.hasParserError() );

  QString errorMsg;
  QDomDocument doc;
  QDomElement filterElem = QgsOgcUtils::expressionToOgcFilter( exp, doc,
                           QgsOgcUtils::GML_3_1_0, QgsOgcUtils::FILTER_OGC_1_1, QStringLiteral( "my_geometry_name" ), srsName, true, false, &errorMsg );

  if ( !errorMsg.isEmpty() )
    qDebug( "ERROR: %s", errorMsg.toAscii().data() );

  QVERIFY( !filterElem.isNull() );

  doc.appendChild( filterElem );

  qDebug( "EXPR: %s", exp.expression().toAscii().data() );
  qDebug( "SRSNAME: %s", srsName.toAscii().data() );
  qDebug( "OGC : %s", doc.toString( -1 ).toAscii().data() );


  QDomElement xmlElem = comparableElement( xmlText );
  QDomElement ogcElem = comparableElement( doc.toString( -1 ) );
  QVERIFY( compareElements( xmlElem, ogcElem ) );
}

void TestQgsOgcUtils::testExpressionToOgcFilterWFS11_data()
{
  QTest::addColumn<QString>( "exprText" );
  QTest::addColumn<QString>( "srsName" );
  QTest::addColumn<QString>( "xmlText" );

  QTest::newRow( "bbox" )
      << QStringLiteral( "intersects_bbox($geometry, geomFromWKT('POLYGON((2 49,2 50,3 50,3 49,2 49))'))" )
      << QStringLiteral( "urn:ogc:def:crs:EPSG::4326" )
      << QString(
        "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\" xmlns:gml=\"http://www.opengis.net/gml\">"
        "<ogc:BBOX>"
        "<ogc:PropertyName>my_geometry_name</ogc:PropertyName>"
        "<gml:Envelope srsName=\"urn:ogc:def:crs:EPSG::4326\">"
        "<gml:lowerCorner>49 2</gml:lowerCorner>"
        "<gml:upperCorner>50 3</gml:upperCorner>"
        "</gml:Envelope>"
        "</ogc:BBOX>"
        "</ogc:Filter>" );
}

void TestQgsOgcUtils::testExpressionToOgcFilterWFS20()
{
  QFETCH( QString, exprText );
  QFETCH( QString, srsName );
  QFETCH( QString, xmlText );

  QgsExpression exp( exprText );
  QVERIFY( !exp.hasParserError() );

  QString errorMsg;
  QDomDocument doc;
  QDomElement filterElem = QgsOgcUtils::expressionToOgcFilter( exp, doc,
                           QgsOgcUtils::GML_3_2_1, QgsOgcUtils::FILTER_FES_2_0, QStringLiteral( "my_geometry_name" ), srsName, true, false, &errorMsg );

  if ( !errorMsg.isEmpty() )
    qDebug( "ERROR: %s", errorMsg.toAscii().data() );

  QVERIFY( !filterElem.isNull() );

  doc.appendChild( filterElem );

  qDebug( "EXPR: %s", exp.expression().toAscii().data() );
  qDebug( "SRSNAME: %s", srsName.toAscii().data() );
  qDebug( "OGC : %s", doc.toString( -1 ).toAscii().data() );

  QDomElement xmlElem = comparableElement( xmlText );
  QDomElement ogcElem = comparableElement( doc.toString( -1 ) );
  QVERIFY( compareElements( xmlElem, ogcElem ) );
}

void TestQgsOgcUtils::testExpressionToOgcFilterWFS20_data()
{
  QTest::addColumn<QString>( "exprText" );
  QTest::addColumn<QString>( "srsName" );
  QTest::addColumn<QString>( "xmlText" );

  QTest::newRow( "=" ) << QStringLiteral( "NAME = 'New York'" ) << QString() << QString(
                         "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\">"
                         "<fes:PropertyIsEqualTo>"
                         "<fes:ValueReference>NAME</fes:ValueReference>"
                         "<fes:Literal>New York</fes:Literal>"
                         "</fes:PropertyIsEqualTo></fes:Filter>" );

  QTest::newRow( "bbox" )
      << QStringLiteral( "intersects_bbox($geometry, geomFromWKT('POLYGON((2 49,2 50,3 50,3 49,2 49))'))" )
      << QStringLiteral( "urn:ogc:def:crs:EPSG::4326" )
      << QString(
        "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\" xmlns:gml=\"http://www.opengis.net/gml/3.2\">"
        "<fes:BBOX>"
        "<fes:ValueReference>my_geometry_name</fes:ValueReference>"
        "<gml:Envelope srsName=\"urn:ogc:def:crs:EPSG::4326\">"
        "<gml:lowerCorner>49 2</gml:lowerCorner>"
        "<gml:upperCorner>50 3</gml:upperCorner>"
        "</gml:Envelope>"
        "</fes:BBOX>"
        "</fes:Filter>" );

  QTest::newRow( "intersects" )
      << QStringLiteral( "intersects($geometry, geomFromWKT('POLYGON((2 49,2 50,3 50,3 49,2 49))'))" )
      << QStringLiteral( "urn:ogc:def:crs:EPSG::4326" )
      << QString(
        "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\" xmlns:gml=\"http://www.opengis.net/gml/3.2\">"
        "<fes:Intersects>"
        "<fes:ValueReference>my_geometry_name</fes:ValueReference>"
        "<gml:Polygon gml:id=\"qgis_id_geom_1\" srsName=\"urn:ogc:def:crs:EPSG::4326\">"
        "<gml:exterior>"
        "<gml:LinearRing>"
        "<gml:posList srsDimension=\"2\">49 2 50 2 50 3 49 3 49 2</gml:posList>"
        "</gml:LinearRing>"
        "</gml:exterior>"
        "</gml:Polygon>"
        "</fes:Intersects>"
        "</fes:Filter>" );
}

Q_DECLARE_METATYPE( QgsOgcUtils::GMLVersion )
Q_DECLARE_METATYPE( QgsOgcUtils::FilterVersion )
Q_DECLARE_METATYPE( QList<QgsOgcUtils::LayerProperties> )

void TestQgsOgcUtils::testSQLStatementToOgcFilter()
{
  QFETCH( QString, statementText );
  QFETCH( QgsOgcUtils::GMLVersion, gmlVersion );
  QFETCH( QgsOgcUtils::FilterVersion, filterVersion );
  QFETCH( QList<QgsOgcUtils::LayerProperties>, layerProperties );
  QFETCH( QString, xmlText );

  QgsSQLStatement statement( statementText );
  if ( !statement.hasParserError() )
  {
    qDebug( "%s", statement.parserErrorString().toAscii().data() );
    QVERIFY( !statement.hasParserError() );
  }

  QString errorMsg;
  QDomDocument doc;
  //QgsOgcUtils::GMLVersion gmlVersion = QgsOgcUtils::GML_3_2_1;
  //QgsOgcUtils::FilterVersion filterVersion = QgsOgcUtils::FILTER_FES_2_0;
  bool honourAxisOrientation = true;
  bool invertAxisOrientation = false;
  //QList<QgsOgcUtils::LayerProperties> layerProperties;
  QDomElement filterElem = QgsOgcUtils::SQLStatementToOgcFilter( statement,
                           doc,
                           gmlVersion,
                           filterVersion,
                           layerProperties,
                           honourAxisOrientation,
                           invertAxisOrientation,
                           QMap<QString, QString>(),
                           &errorMsg );

  if ( !errorMsg.isEmpty() )
    qDebug( "ERROR: %s", errorMsg.toAscii().data() );

  QVERIFY( !filterElem.isNull() );

  doc.appendChild( filterElem );

  qDebug( "SQL:    %s", statement.statement().toAscii().data() );
  qDebug( "GML:    %s", gmlVersion == QgsOgcUtils::GML_2_1_2 ? "2.1.2" :
          gmlVersion == QgsOgcUtils::GML_3_1_0 ? "3.1.0" :
          gmlVersion == QgsOgcUtils::GML_3_2_1 ? "3.2.1" : "unknown" );
  qDebug( "FILTER: %s", filterVersion == QgsOgcUtils::FILTER_OGC_1_0 ? "OGC 1.0" :
          filterVersion == QgsOgcUtils::FILTER_OGC_1_1 ? "OGC 1.1" :
          filterVersion == QgsOgcUtils::FILTER_FES_2_0 ? "FES 2.0" : "unknown" );
  qDebug( "OGC :   %s", doc.toString( -1 ).toAscii().data() );

  QDomElement xmlElem = comparableElement( xmlText );
  QDomElement ogcElem = comparableElement( doc.toString( -1 ) );
  QVERIFY( compareElements( xmlElem, ogcElem ) );
}

void TestQgsOgcUtils::testSQLStatementToOgcFilter_data()
{
  QList<QgsOgcUtils::LayerProperties> layerProperties;

  QTest::addColumn<QString>( "statementText" );
  QTest::addColumn<QgsOgcUtils::GMLVersion>( "gmlVersion" );
  QTest::addColumn<QgsOgcUtils::FilterVersion>( "filterVersion" );
  QTest::addColumn< QList<QgsOgcUtils::LayerProperties> >( "layerProperties" );
  QTest::addColumn<QString>( "xmlText" );

  QTest::newRow( "= 1.0" ) << QStringLiteral( "SELECT * FROM t WHERE NAME = 'New York'" ) <<
                           QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties <<
                           QString(
                             "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                             "<ogc:PropertyIsEqualTo>"
                             "<ogc:PropertyName>NAME</ogc:PropertyName>"
                             "<ogc:Literal>New York</ogc:Literal>"
                             "</ogc:PropertyIsEqualTo>"
                             "</ogc:Filter>" );

  QTest::newRow( "= 2.0" ) << QStringLiteral( "SELECT * FROM t WHERE NAME = 'New York'" ) <<
                           QgsOgcUtils::GML_3_2_1 << QgsOgcUtils::FILTER_FES_2_0 << layerProperties <<
                           QString(
                             "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\">"
                             "<fes:PropertyIsEqualTo>"
                             "<fes:ValueReference>NAME</fes:ValueReference>"
                             "<fes:Literal>New York</fes:Literal>"
                             "</fes:PropertyIsEqualTo>"
                             "</fes:Filter>" );

  QTest::newRow( ">" ) << QStringLiteral( "SELECT * FROM t WHERE COUNT > 3" ) <<
                       QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties <<
                       QString(
                         "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                         "<ogc:PropertyIsGreaterThan>"
                         "<ogc:PropertyName>COUNT</ogc:PropertyName>"
                         "<ogc:Literal>3</ogc:Literal>"
                         "</ogc:PropertyIsGreaterThan></ogc:Filter>" );

  QTest::newRow( "and+or" ) << QStringLiteral( "SELECT * FROM t WHERE (FIELD1 <= 10 OR FIELD1 > 20) AND STATUS >= 1.5" ) <<
                            QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties << QString(
                              "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                              "<ogc:And>"
                              "<ogc:Or>"
                              "<ogc:PropertyIsLessThanOrEqualTo>"
                              "<ogc:PropertyName>FIELD1</ogc:PropertyName>"
                              "<ogc:Literal>10</ogc:Literal>"
                              "</ogc:PropertyIsLessThanOrEqualTo>"
                              "<ogc:PropertyIsGreaterThan>"
                              "<ogc:PropertyName>FIELD1</ogc:PropertyName>"
                              "<ogc:Literal>20</ogc:Literal>"
                              "</ogc:PropertyIsGreaterThan>"
                              "</ogc:Or>"
                              "<ogc:PropertyIsGreaterThanOrEqualTo>"
                              "<ogc:PropertyName>STATUS</ogc:PropertyName>"
                              "<ogc:Literal>1.5</ogc:Literal>"
                              "</ogc:PropertyIsGreaterThanOrEqualTo>"
                              "</ogc:And>"
                              "</ogc:Filter>" );

  QTest::newRow( "is null" ) << QStringLiteral( "SELECT * FROM t WHERE A IS NULL" ) <<
                             QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties << QString(
                               "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                               "<ogc:PropertyIsNull>"
                               "<ogc:PropertyName>A</ogc:PropertyName>"
                               "</ogc:PropertyIsNull>"
                               "</ogc:Filter>" );

  QTest::newRow( "is not null" ) << QStringLiteral( "SELECT * FROM t WHERE A IS NOT NULL" ) <<
                                 QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties << QString(
                                   "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                                   "<ogc:Not>"
                                   "<ogc:PropertyIsNull>"
                                   "<ogc:PropertyName>A</ogc:PropertyName>"
                                   "</ogc:PropertyIsNull>"
                                   "</ogc:Not>"
                                   "</ogc:Filter>" );

  QTest::newRow( "in" ) << QStringLiteral( "SELECT * FROM t WHERE A IN (10,20,30)" ) <<
                        QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties << QString(
                          "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                          "<ogc:Or>"
                          "<ogc:PropertyIsEqualTo>"
                          "<ogc:PropertyName>A</ogc:PropertyName>"
                          "<ogc:Literal>10</ogc:Literal>"
                          "</ogc:PropertyIsEqualTo>"
                          "<ogc:PropertyIsEqualTo>"
                          "<ogc:PropertyName>A</ogc:PropertyName>"
                          "<ogc:Literal>20</ogc:Literal>"
                          "</ogc:PropertyIsEqualTo>"
                          "<ogc:PropertyIsEqualTo>"
                          "<ogc:PropertyName>A</ogc:PropertyName>"
                          "<ogc:Literal>30</ogc:Literal>"
                          "</ogc:PropertyIsEqualTo>"
                          "</ogc:Or>"
                          "</ogc:Filter>" );

  QTest::newRow( "not in" ) << QStringLiteral( "SELECT * FROM t WHERE A NOT IN (10,20,30)" ) <<
                            QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties << QString(
                              "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                              "<ogc:Not>"
                              "<ogc:Or>"
                              "<ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyName>A</ogc:PropertyName>"
                              "<ogc:Literal>10</ogc:Literal>"
                              "</ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyName>A</ogc:PropertyName>"
                              "<ogc:Literal>20</ogc:Literal>"
                              "</ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyIsEqualTo>"
                              "<ogc:PropertyName>A</ogc:PropertyName>"
                              "<ogc:Literal>30</ogc:Literal>"
                              "</ogc:PropertyIsEqualTo>"
                              "</ogc:Or>"
                              "</ogc:Not>"
                              "</ogc:Filter>" );

  QTest::newRow( "between" ) << QStringLiteral( "SELECT * FROM t WHERE A BETWEEN 1 AND 2" ) <<
                             QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties << QString(
                               "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                               "<ogc:PropertyIsBetween>"
                               "<ogc:PropertyName>A</ogc:PropertyName>"
                               "<ogc:LowerBoundary><ogc:Literal>1</ogc:Literal></ogc:LowerBoundary>"
                               "<ogc:UpperBoundary><ogc:Literal>2</ogc:Literal></ogc:UpperBoundary>"
                               "</ogc:PropertyIsBetween>"
                               "</ogc:Filter>" );

  QTest::newRow( "not between" ) << QStringLiteral( "SELECT * FROM t WHERE A NOT BETWEEN 1 AND 2" ) <<
                                 QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties << QString(
                                   "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                                   "<ogc:Not>"
                                   "<ogc:PropertyIsBetween>"
                                   "<ogc:PropertyName>A</ogc:PropertyName>"
                                   "<ogc:LowerBoundary><ogc:Literal>1</ogc:Literal></ogc:LowerBoundary>"
                                   "<ogc:UpperBoundary><ogc:Literal>2</ogc:Literal></ogc:UpperBoundary>"
                                   "</ogc:PropertyIsBetween>"
                                   "</ogc:Not>"
                                   "</ogc:Filter>" );

  QTest::newRow( "intersects + wkt" ) << QStringLiteral( "SELECT * FROM t WHERE ST_Intersects(geom, ST_GeometryFromText('POINT (5 6)'))" ) <<
                                      QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties << QString(
                                        "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\" xmlns:gml=\"http://www.opengis.net/gml\">"
                                        "<ogc:Intersects>"
                                        "<ogc:PropertyName>geom</ogc:PropertyName>"
                                        "<gml:Point><gml:coordinates ts=\" \" cs=\",\">5,6</gml:coordinates></gml:Point>"
                                        "</ogc:Intersects>"
                                        "</ogc:Filter>" );

  QTest::newRow( "contains + gml" ) << QStringLiteral( "SELECT * FROM t WHERE ST_Contains(geom, ST_GeomFromGML('<gml:Point xmlns:gml=\"http://www.opengis.net/gml\"><gml:coordinates cs=\",\" ts=\" \">5,6</gml:coordinates></gml:Point>'))" ) <<
                                    QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties << QString(
                                      "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\" xmlns:gml=\"http://www.opengis.net/gml\">"
                                      "<ogc:Contains>"
                                      "<ogc:PropertyName>geom</ogc:PropertyName>"
                                      "<gml:Point xmlns:gml=\"http://www.opengis.net/gml\"><gml:coordinates xmlns:gml=\"http://www.opengis.net/gml\" ts=\" \" cs=\",\">5,6</gml:coordinates></gml:Point>"
                                      "</ogc:Contains>"
                                      "</ogc:Filter>" );

  QTest::newRow( "abs" ) << QStringLiteral( "SELECT * FROM t WHERE ABS(x) < 5" ) <<
                         QgsOgcUtils::GML_2_1_2 << QgsOgcUtils::FILTER_OGC_1_0 << layerProperties << QString(
                           "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">"
                           "<ogc:PropertyIsLessThan>"
                           "<ogc:Function name=\"ABS\">"
                           "<ogc:PropertyName>x</ogc:PropertyName>"
                           "</ogc:Function>"
                           "<ogc:Literal>5</ogc:Literal>"
                           "</ogc:PropertyIsLessThan>"
                           "</ogc:Filter>" );

  QTest::newRow( "bbox + wkt + explicit srs" ) << QStringLiteral( "SELECT * FROM t WHERE BBOX(geom, ST_MakeEnvelope(2.2, 49, 3, 50, 4326))" ) <<
      QgsOgcUtils::GML_3_1_0 << QgsOgcUtils::FILTER_OGC_1_1 << layerProperties << QString(
        "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\" xmlns:gml=\"http://www.opengis.net/gml\">"
        "<ogc:BBOX>"
        "<ogc:PropertyName>geom</ogc:PropertyName>"
        "<gml:Envelope srsName=\"urn:ogc:def:crs:EPSG::4326\">"
        "<gml:lowerCorner>49 2.2</gml:lowerCorner>"
        "<gml:upperCorner>50 3</gml:upperCorner>"
        "</gml:Envelope>"
        "</ogc:BBOX>"
        "</ogc:Filter>" );

  QTest::newRow( "intersects + wkt + explicit srs" ) << QStringLiteral( "SELECT * FROM t WHERE ST_Intersects(geom, ST_GeometryFromText('POINT (5 6)', 'urn:ogc:def:crs:EPSG::4326'))" ) <<
      QgsOgcUtils::GML_3_2_1 << QgsOgcUtils::FILTER_FES_2_0 << layerProperties << QString(
        "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\" xmlns:gml=\"http://www.opengis.net/gml/3.2\">"
        "<fes:Intersects>"
        "<fes:ValueReference>geom</fes:ValueReference>"
        "<gml:Point gml:id=\"qgis_id_geom_1\" srsName=\"urn:ogc:def:crs:EPSG::4326\">"
        "<gml:pos srsDimension=\"2\">6 5</gml:pos>"
        "</gml:Point>"
        "</fes:Intersects>"
        "</fes:Filter>" );

  QTest::newRow( "intersects + wkt + explicit srs int" ) << QStringLiteral( "SELECT * FROM t WHERE ST_Intersects(geom, ST_GeometryFromText('POINT (5 6)', 4326))" ) <<
      QgsOgcUtils::GML_3_2_1 << QgsOgcUtils::FILTER_FES_2_0 << layerProperties << QString(
        "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\" xmlns:gml=\"http://www.opengis.net/gml/3.2\">"
        "<fes:Intersects>"
        "<fes:ValueReference>geom</fes:ValueReference>"
        "<gml:Point gml:id=\"qgis_id_geom_1\" srsName=\"urn:ogc:def:crs:EPSG::4326\">"
        "<gml:pos srsDimension=\"2\">6 5</gml:pos>"
        "</gml:Point>"
        "</fes:Intersects>"
        "</fes:Filter>" );

  QTest::newRow( "dwithin + wkt" ) << QStringLiteral( "SELECT * FROM t WHERE ST_DWithin(geom, ST_GeometryFromText('POINT (5 6)', 'urn:ogc:def:crs:EPSG::4326'), '3 m')" ) <<
                                   QgsOgcUtils::GML_3_2_1 << QgsOgcUtils::FILTER_FES_2_0 << layerProperties << QString(
                                     "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\" xmlns:gml=\"http://www.opengis.net/gml/3.2\">"
                                     "<fes:DWithin>"
                                     "<fes:ValueReference>geom</fes:ValueReference>"
                                     "<gml:Point gml:id=\"qgis_id_geom_1\" srsName=\"urn:ogc:def:crs:EPSG::4326\">"
                                     "<gml:pos srsDimension=\"2\">6 5</gml:pos>"
                                     "</gml:Point>"
                                     "<fes:Distance uom=\"m\">3</fes:Distance>"
                                     "</fes:DWithin>"
                                     "</fes:Filter>" );

  QList<QgsOgcUtils::LayerProperties> layerProperties4326_FES20;
  QgsOgcUtils::LayerProperties prop;
  prop.mSRSName = QStringLiteral( "urn:ogc:def:crs:EPSG::4326" );
  prop.mGeometryAttribute = QStringLiteral( "geom" );
  layerProperties4326_FES20.append( prop );

  QTest::newRow( "intersects + wkt + implicit SRS" ) << QStringLiteral( "SELECT * FROM t WHERE ST_Intersects(geom, ST_GeometryFromText('POINT (5 6)'))" ) <<
      QgsOgcUtils::GML_3_2_1 << QgsOgcUtils::FILTER_FES_2_0 << layerProperties4326_FES20 << QString(
        "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\" xmlns:gml=\"http://www.opengis.net/gml/3.2\">"
        "<fes:Intersects>"
        "<fes:ValueReference>geom</fes:ValueReference>"
        "<gml:Point gml:id=\"qgis_id_geom_1\" srsName=\"urn:ogc:def:crs:EPSG::4326\">"
        "<gml:pos srsDimension=\"2\">6 5</gml:pos>"
        "</gml:Point>"
        "</fes:Intersects>"
        "</fes:Filter>" );

  QTest::newRow( "intersects join 2.0" ) << QStringLiteral( "SELECT * FROM t, t2 WHERE ST_Intersects(t.geom, t2.geom)" ) <<
                                         QgsOgcUtils::GML_3_2_1 << QgsOgcUtils::FILTER_FES_2_0 << layerProperties << QString(
                                             "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\">"
                                             "<fes:Intersects>"
                                             "<fes:ValueReference>t/geom</fes:ValueReference>"
                                             "<fes:ValueReference>t2/geom</fes:ValueReference>"
                                             "</fes:Intersects>"
                                             "</fes:Filter>" );

  QTest::newRow( "attrib join USING 2.0" ) << QStringLiteral( "SELECT * FROM t JOIN t2 USING (a)" ) <<
      QgsOgcUtils::GML_3_2_1 << QgsOgcUtils::FILTER_FES_2_0 << layerProperties << QString(
        "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\">"
        "<fes:PropertyIsEqualTo>"
        "<fes:ValueReference>t/a</fes:ValueReference>"
        "<fes:ValueReference>t2/a</fes:ValueReference>"
        "</fes:PropertyIsEqualTo>"
        "</fes:Filter>" );

  QTest::newRow( "attrib join multi USING 2.0" ) << QStringLiteral( "SELECT * FROM t JOIN t2 USING (a, b)" ) <<
      QgsOgcUtils::GML_3_2_1 << QgsOgcUtils::FILTER_FES_2_0 << layerProperties << QString(
        "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\">"
        "<fes:And>"
        "<fes:PropertyIsEqualTo>"
        "<fes:ValueReference>t/a</fes:ValueReference>"
        "<fes:ValueReference>t2/a</fes:ValueReference>"
        "</fes:PropertyIsEqualTo>"
        "<fes:PropertyIsEqualTo>"
        "<fes:ValueReference>t/b</fes:ValueReference>"
        "<fes:ValueReference>t2/b</fes:ValueReference>"
        "</fes:PropertyIsEqualTo>"
        "</fes:And>"
        "</fes:Filter>" );

  QTest::newRow( "attrib join ON 2.0" ) << QStringLiteral( "SELECT * FROM t aliased_t JOIN t2 aliasted_t2 ON aliased_t.a = aliasted_t2.b" ) <<
                                        QgsOgcUtils::GML_3_2_1 << QgsOgcUtils::FILTER_FES_2_0 << layerProperties << QString(
                                          "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\">"
                                          "<fes:PropertyIsEqualTo>"
                                          "<fes:ValueReference>t/a</fes:ValueReference>"
                                          "<fes:ValueReference>t2/b</fes:ValueReference>"
                                          "</fes:PropertyIsEqualTo>"
                                          "</fes:Filter>" );

  QTest::newRow( "attrib multi join 2.0" ) << QStringLiteral( "SELECT * FROM t aliased_t JOIN t2 aliasted_t2 ON aliased_t.a = aliasted_t2.b JOIN t3 USING (c)" ) <<
      QgsOgcUtils::GML_3_2_1 << QgsOgcUtils::FILTER_FES_2_0 << layerProperties << QString(
        "<fes:Filter xmlns:fes=\"http://www.opengis.net/fes/2.0\">"
        "<fes:And>"
        "<fes:PropertyIsEqualTo>"
        "<fes:ValueReference>t/a</fes:ValueReference>"
        "<fes:ValueReference>t2/b</fes:ValueReference>"
        "</fes:PropertyIsEqualTo>"
        "<fes:PropertyIsEqualTo>"
        "<fes:ValueReference>t2/c</fes:ValueReference>"
        "<fes:ValueReference>t3/c</fes:ValueReference>"
        "</fes:PropertyIsEqualTo>"
        "</fes:And>"
        "</fes:Filter>" );
}

QGSTEST_MAIN( TestQgsOgcUtils )
#include "testqgsogcutils.moc"
