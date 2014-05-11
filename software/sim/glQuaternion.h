/**
* @file glQuaternion.h
*
* @brief Contains interface for the glQuaternion class
*/

#include <math.h>

#define PI			3.14159265358979323846

/**
* @brief Quaternion used to represent a rotation
*/
class glQuaternion
{
public:
   glQuaternion operator *(glQuaternion q);
   void CreateMatrix(GLfloat *pMatrix);
   void CreateFromAxisAngle(GLfloat x, GLfloat y, GLfloat z, GLfloat degrees);
   glQuaternion();
   virtual ~glQuaternion();

private:
   GLfloat m_w;
   GLfloat m_z;
   GLfloat m_y;
   GLfloat m_x;
};


