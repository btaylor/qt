/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <QtDeclarative/private/qdeclarativelistmodel_p.h>
#include <QtDeclarative/private/qdeclarativeexpression_p.h>
#include <QDeclarativeComponent>
#include <QDebug>

class tst_QDeclarativeListModel : public QObject
{
    Q_OBJECT
public:
    tst_QDeclarativeListModel() {}

private slots:
    void static_types();
    void static_types_data();
    void static_i18n();
    void static_nestedElements();
    void static_nestedElements_data();
    void dynamic_data();
    void dynamic();
    void error_data();
    void error();
};

void tst_QDeclarativeListModel::static_i18n()
{
    QString expect = QString::fromUtf8("na\303\257ve");
    QString componentStr = "import Qt 4.6\nListModel { ListElement { prop1: \""+expect+"\" } }";
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toUtf8(), QUrl::fromLocalFile(""));
    QDeclarativeListModel *obj = qobject_cast<QDeclarativeListModel*>(component.create());
    QVERIFY(obj != 0);
    QString prop = obj->get(0).property(QLatin1String("prop1")).toString();
    QCOMPARE(prop,expect);
    delete obj;
}

void tst_QDeclarativeListModel::static_nestedElements()
{
    QFETCH(int, elementCount);

    QStringList elements;
    for (int i=0; i<elementCount; i++) 
        elements.append("ListElement { a: 1; b: 2 }");
    QString elementsStr = elements.join(",\n") + "\n";

    QString componentStr = 
        "import Qt 4.6\n"
        "ListModel {\n"
        "   ListElement {\n"
        "       attributes: [\n";
    componentStr += elementsStr.toUtf8().constData();
    componentStr += 
        "       ]\n"
        "   }\n"
        "}";

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toUtf8(), QUrl::fromLocalFile(""));

    QDeclarativeListModel *obj = qobject_cast<QDeclarativeListModel*>(component.create());
    QVERIFY(obj != 0);

    QScriptValue prop = obj->get(0).property(QLatin1String("attributes")).property(QLatin1String("count"));
    QVERIFY(prop.isNumber());
    QCOMPARE(prop.toInt32(), qint32(elementCount));

    delete obj;
}

void tst_QDeclarativeListModel::static_nestedElements_data()
{
    QTest::addColumn<int>("elementCount");

    QTest::newRow("0 items") << 0;
    QTest::newRow("1 item") << 1;
    QTest::newRow("2 items") << 2;
    QTest::newRow("many items") << 5;
}

void tst_QDeclarativeListModel::dynamic_data()
{
    QTest::addColumn<QString>("script");
    QTest::addColumn<int>("result");
    QTest::addColumn<QString>("warning");

    // Simple flat model

    QTest::newRow("count") << "count" << 0 << "";

    QTest::newRow("get1") << "{get(0)}" << 0 << "QML ListModel (unknown location) get: index 0 out of range";
    QTest::newRow("get2") << "{get(-1)}" << 0 << "QML ListModel (unknown location) get: index -1 out of range";

    QTest::newRow("append1") << "{append({'foo':123});count}" << 1 << "";
    QTest::newRow("append2") << "{append({'foo':123,'bar':456});count}" << 1 << "";
    QTest::newRow("append3a") << "{append({'foo':123});append({'foo':456});get(0).foo}" << 123 << "";
    QTest::newRow("append3b") << "{append({'foo':123});append({'foo':456});get(1).foo}" << 456 << "";
    QTest::newRow("append4a") << "{append(123)}" << 0 << "QML ListModel (unknown location) append: value is not an object";
    QTest::newRow("append4b") << "{append([1,2,3])}" << 0 << "QML ListModel (unknown location) append: value is not an object";

    QTest::newRow("clear1") << "{append({'foo':456});clear();count}" << 0 << "";
    QTest::newRow("clear2") << "{append({'foo':123});append({'foo':456});clear();count}" << 0 << "";
    QTest::newRow("clear2") << "{append({'foo':123});clear();get(0).foo}" << 0 << "QML ListModel (unknown location) get: index 0 out of range";

    QTest::newRow("remove1") << "{append({'foo':123});remove(0);count}" << 0 << "";
    QTest::newRow("remove2a") << "{append({'foo':123});append({'foo':456});remove(0);count}" << 1 << "";
    QTest::newRow("remove2b") << "{append({'foo':123});append({'foo':456});remove(0);get(0).foo}" << 456 << "";
    QTest::newRow("remove2c") << "{append({'foo':123});append({'foo':456});remove(1);get(0).foo}" << 123 << "";
    QTest::newRow("remove3") << "{append({'foo':123});remove(0);get(0).foo}" << 0 << "QML ListModel (unknown location) get: index 0 out of range";
    QTest::newRow("remove3a") << "{append({'foo':123});remove(-1)}" << 0 << "QML ListModel (unknown location) remove: index -1 out of range";
    QTest::newRow("remove4a") << "{remove(0)}" << 0 << "QML ListModel (unknown location) remove: index 0 out of range";
    QTest::newRow("remove4b") << "{append({'foo':123});remove(0);remove(0)}" << 0 << "QML ListModel (unknown location) remove: index 0 out of range";
    QTest::newRow("remove4c") << "{append({'foo':123});remove(1)}" << 0 << "QML ListModel (unknown location) remove: index 1 out of range";

    QTest::newRow("insert1") << "{insert(0,{'foo':123});count}" << 1 << "";
    QTest::newRow("insert2") << "{insert(1,{'foo':123});count}" << 0 << "QML ListModel (unknown location) insert: index 1 out of range";
    QTest::newRow("insert3a") << "{append({'foo':123});insert(1,{'foo':456});count}" << 2 << "";
    QTest::newRow("insert3b") << "{append({'foo':123});insert(1,{'foo':456});get(0).foo}" << 123 << "";
    QTest::newRow("insert3c") << "{append({'foo':123});insert(1,{'foo':456});get(1).foo}" << 456 << "";
    QTest::newRow("insert3d") << "{append({'foo':123});insert(0,{'foo':456});get(0).foo}" << 456 << "";
    QTest::newRow("insert3e") << "{append({'foo':123});insert(0,{'foo':456});get(1).foo}" << 123 << "";
    QTest::newRow("insert4") << "{append({'foo':123});insert(-1,{'foo':456})}" << 0 << "QML ListModel (unknown location) insert: index -1 out of range";
    QTest::newRow("insert5a") << "{insert(0,123)}" << 0 << "QML ListModel (unknown location) insert: value is not an object";
    QTest::newRow("insert5b") << "{insert(0,[1,2,3])}" << 0 << "QML ListModel (unknown location) insert: value is not an object";

    QTest::newRow("set1") << "{append({'foo':123});set(0,{'foo':456});count}" << 1 << "";
    QTest::newRow("set2") << "{append({'foo':123});set(0,{'foo':456});get(0).foo}" << 456 << "";
    QTest::newRow("set3a") << "{append({'foo':123,'bar':456});set(0,{'foo':999});get(0).foo}" << 999 << "";
    QTest::newRow("set3b") << "{append({'foo':123,'bar':456});set(0,{'foo':999});get(0).bar}" << 456 << "";
    QTest::newRow("set4a") << "{set(0,{'foo':456})}" << 0 << "QML ListModel (unknown location) set: index 0 out of range";
    QTest::newRow("set4c") << "{set(-1,{'foo':456})}" << 0 << "QML ListModel (unknown location) set: index -1 out of range";
    QTest::newRow("set5a") << "{append({'foo':123,'bar':456});set(0,123)}" << 0 << "QML ListModel (unknown location) set: value is not an object";
    QTest::newRow("set5b") << "{append({'foo':123,'bar':456});set(0,[1,2,3])}" << 0 << "QML ListModel (unknown location) set: value is not an object";
    QTest::newRow("set6") << "{append({'foo':123});set(1,{'foo':456});count}" << 2 << "";

    QTest::newRow("setprop1") << "{append({'foo':123});setProperty(0,'foo',456);count}" << 1 << "";
    QTest::newRow("setprop2") << "{append({'foo':123});setProperty(0,'foo',456);get(0).foo}" << 456 << "";
    QTest::newRow("setprop3a") << "{append({'foo':123,'bar':456});setProperty(0,'foo',999);get(0).foo}" << 999 << "";
    QTest::newRow("setprop3b") << "{append({'foo':123,'bar':456});setProperty(0,'foo',999);get(0).bar}" << 456 << "";
    QTest::newRow("setprop4a") << "{setProperty(0,'foo',456)}" << 0 << "QML ListModel (unknown location) set: index 0 out of range";
    QTest::newRow("setprop4b") << "{setProperty(-1,'foo',456)}" << 0 << "QML ListModel (unknown location) set: index -1 out of range";
    QTest::newRow("setprop4c") << "{append({'foo':123,'bar':456});setProperty(1,'foo',456)}" << 0 << "QML ListModel (unknown location) set: index 1 out of range";
    QTest::newRow("setprop5") << "{append({'foo':123,'bar':456});append({'foo':111});setProperty(1,'bar',222);get(1).bar}" << 222 << "";

    QTest::newRow("move1a") << "{append({'foo':123});append({'foo':456});move(0,1,1);count}" << 2 << "";
    QTest::newRow("move1b") << "{append({'foo':123});append({'foo':456});move(0,1,1);get(0).foo}" << 456 << "";
    QTest::newRow("move1c") << "{append({'foo':123});append({'foo':456});move(0,1,1);get(1).foo}" << 123 << "";
    QTest::newRow("move1d") << "{append({'foo':123});append({'foo':456});move(1,0,1);get(0).foo}" << 456 << "";
    QTest::newRow("move1e") << "{append({'foo':123});append({'foo':456});move(1,0,1);get(1).foo}" << 123 << "";
    QTest::newRow("move2a") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);count}" << 3 << "";
    QTest::newRow("move2b") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(0).foo}" << 789 << "";
    QTest::newRow("move2c") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(1).foo}" << 123 << "";
    QTest::newRow("move2d") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(2).foo}" << 456 << "";
    QTest::newRow("move3a") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,0,3)}" << 0 << "QML ListModel (unknown location) move: out of range";
    QTest::newRow("move3b") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,-1,1)}" << 0 << "QML ListModel (unknown location) move: out of range";
    QTest::newRow("move3c") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,0,-1)}" << 0 << "QML ListModel (unknown location) move: out of range";
    QTest::newRow("move3d") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,3,1)}" << 0 << "QML ListModel (unknown location) move: out of range";

    // Structured model

    QTest::newRow("listprop1a") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});count}" << 1 << "";
    QTest::newRow("listprop1b") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});get(0).bars.get(1).a}" << 2 << "";
    QTest::newRow("listprop2a") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});get(0).bars.append({'a':4});get(0).bars.get(3).a}" << 4 << "";
}

void tst_QDeclarativeListModel::dynamic()
{
    QFETCH(QString, script);
    QFETCH(int, result);
    QFETCH(QString, warning);

    QDeclarativeEngine engine;
    QDeclarativeListModel model;
    QDeclarativeEngine::setContextForObject(&model,engine.rootContext());
    engine.rootContext()->addDefaultObject(&model);
    QDeclarativeExpression e(engine.rootContext(), script, &model);
    if (!warning.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());
    int actual = e.value().toInt();
    if (e.hasError())
        qDebug() << e.error(); // errors not expected
    QVERIFY(!e.hasError());
    QCOMPARE(actual,result);
}

void tst_QDeclarativeListModel::static_types_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QVariant>("value");

    QTest::newRow("string")
        << "ListElement { foo: \"bar\" }"
        << QVariant(QString("bar"));

    QTest::newRow("real")
        << "ListElement { foo: 10.5 }"
        << QVariant(10.5);

    QTest::newRow("real0")
        << "ListElement { foo: 0 }"
        << QVariant(double(0));

    QTest::newRow("bool")
        << "ListElement { foo: false }"
        << QVariant(false);

    QTest::newRow("bool")
        << "ListElement { foo: true }"
        << QVariant(true);

    QTest::newRow("enum")
        << "ListElement { foo: Text.AlignHCenter }"
        << QVariant("QTBUG-5974:ListElement: constant script support for property value");
}

void tst_QDeclarativeListModel::static_types()
{
    QFETCH(QString, qml);
    QFETCH(QVariant, value);

    qml = "import Qt 4.6\nListModel { " + qml + " }";

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(qml.toUtf8(),
                      QUrl::fromLocalFile(QString("dummy.qml")));

    if (value.toString().startsWith("QTBUG-"))
        QEXPECT_FAIL("",value.toString().toLatin1(),Abort);

    QVERIFY(!component.isError());

    QDeclarativeListModel *obj = qobject_cast<QDeclarativeListModel*>(component.create());
    QVERIFY(obj != 0);

    QScriptValue actual = obj->get(0).property(QLatin1String("foo"));

    QCOMPARE(actual.isString(), value.type() == QVariant::String);
    QCOMPARE(actual.isBoolean(), value.type() == QVariant::Bool);
    QCOMPARE(actual.isNumber(), value.type() == QVariant::Double);

    QCOMPARE(actual.toString(), value.toString());

    delete obj;
}

void tst_QDeclarativeListModel::error_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("error");

    QTest::newRow("id not allowed in ListElement")
        << "import Qt 4.6\nListModel { ListElement { id: fred } }"
        << "ListElement: cannot use reserved \"id\" property";

    QTest::newRow("id allowed in ListModel")
        << "import Qt 4.6\nListModel { id:model }"
        << "";

    QTest::newRow("random properties not allowed in ListModel")
        << "import Qt 4.6\nListModel { foo:123 }"
        << "ListModel: undefined property 'foo'";

    QTest::newRow("random properties allowed in ListElement")
        << "import Qt 4.6\nListModel { ListElement { foo:123 } }"
        << "";

    QTest::newRow("bindings not allowed in ListElement")
        << "import Qt 4.6\nRectangle { id: rect; ListModel { ListElement { foo: rect.color } } }"
        << "ListElement: cannot use script for property value"; // but note QTBUG-5974

    QTest::newRow("random object list properties allowed in ListElement")
        << "import Qt 4.6\nListModel { ListElement { foo: [ ListElement { bar: 123 } ] } }"
        << "";

    QTest::newRow("default properties not allowed in ListElement")
        << "import Qt 4.6\nListModel { ListElement { Item { } } }"
        << "QTBUG-6082 ListElement should not allow child objects";
}

void tst_QDeclarativeListModel::error()
{
    QFETCH(QString, qml);
    QFETCH(QString, error);

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(qml.toUtf8(),
                      QUrl::fromLocalFile(QString("dummy.qml")));
    if (error.isEmpty()) {
        QVERIFY(!component.isError());
    } else {
        if (error.startsWith(QLatin1String("QTBUG-")))
            QEXPECT_FAIL("",error.toLatin1(),Abort);
        QVERIFY(component.isError());
        QList<QDeclarativeError> errors = component.errors();
        QCOMPARE(errors.count(),1);
        QCOMPARE(errors.at(0).description(),error);
    }
}

QTEST_MAIN(tst_QDeclarativeListModel)

#include "tst_qdeclarativelistmodel.moc"