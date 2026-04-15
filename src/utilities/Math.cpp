#include "../Includes.h"

float CMath::GetFOV( QAngle angViewAngle, QAngle angAimAngle )
{
	Vector vAim = angViewAngle.ToVec();
	Vector vAng = angAimAngle.ToVec();

	float flLen = vAim.LengthSqr();
	if (flLen < 0.0001f) return 180.f;

	float flDot = std::clamp(vAim.DotProduct(vAng) / flLen, -1.f, 1.f);
	return M_RAD2DEG( acos( flDot ) );
}

QAngle CMath::CalcAngle( const Vector& vecStart, const Vector& vecEnd )
{
	Vector vecDelta = vecEnd - vecStart;
	QAngle angView = vecDelta.ToAngles();
	angView.Normalize( );
	return angView;
}