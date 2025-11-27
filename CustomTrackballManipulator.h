#ifndef CUSTOMTRACKBALLMANIPULATOR_H
#define CUSTOMTRACKBALLMANIPULATOR_H

#include <osgGA/TrackballManipulator>
#include <osg/Quat>
#include <osg/Vec3>
#include <osg/Matrixd>

class CustomTrackballManipulator : public osgGA::TrackballManipulator
{
public:
    CustomTrackballManipulator();
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);
    
    // 添加setMouseWheelZoomFactor方法
    void setMouseWheelZoomFactor(double factor);
    
protected:
    bool performMovementRightMouseButton(const double eventTimeDelta, const double dx, const double dy);
    bool performMovementLeftMouseButton(const double eventTimeDelta, const double dx, const double dy);
    bool performMovementMiddleMouseButton(const double eventTimeDelta, const double dx, const double dy);
    
    osg::Vec3d getSideVector(const osg::Vec3d& up, const osg::Vec3d& forward) const;
    
private:
    bool m_dragging;
};

#endif // CUSTOMTRACKBALLMANIPULATOR_H