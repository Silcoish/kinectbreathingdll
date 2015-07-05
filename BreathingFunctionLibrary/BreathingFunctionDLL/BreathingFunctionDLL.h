#ifndef BREATHINGFUNCTIONDLL_H
#define BREATHINGFUNCTIONDLL_H
#include <stdexcept>
//#include "stdafx.h"
#include "resources.h";
#endif // !BREATHINGFUNCTIONDLL_H

namespace BreathingFunctions
{
	extern "C" {
		struct BreathingVariables
		{
			float calibration = 0.0f;
			float previousReading = 0.0f;
			float currentReading = 0.0f;

			float logY = 0.0f;
			int sampleSize = 50;
			int frameCount = 0;
			float buffer = 0.2f;
		};

		class BreathingClass
		{
			static const int        cDepthWidth = 512;
			static const int        cDepthHeight = 424;

			HWND                    m_hWnd;
			INT64                   m_nStartTime;
			INT64                   m_nLastCounter;
			double                  m_fFreq;
			INT64                   m_nNextStatusTime;
			DWORD                   m_nFramesSinceUpdate;

			// Current Kinect
			IKinectSensor*          m_pKinectSensor;
			ICoordinateMapper*      m_pCoordinateMapper;

			// Body reader
			IBodyFrameReader*       m_pBodyFrameReader;

			// Direct2D
			ID2D1Factory*           m_pD2DFactory;

			// Body/hand drawing
			ID2D1HwndRenderTarget*  m_pRenderTarget;
			ID2D1SolidColorBrush*   m_pBrushJointTracked;
			ID2D1SolidColorBrush*   m_pBrushJointInferred;
			ID2D1SolidColorBrush*   m_pBrushBoneTracked;
			ID2D1SolidColorBrush*   m_pBrushBoneInferred;
			ID2D1SolidColorBrush*   m_pBrushHandClosed;
			ID2D1SolidColorBrush*   m_pBrushHandOpen;
			ID2D1SolidColorBrush*   m_pBrushHandLasso;

			BreathingVariables breathingVars;

			__declspec(dllexport) void ProcessBody(INT64 nTime, int nBodyCount, IBody** ppBodies);
			__declspec(dllexport) HRESULT EnsureDirect2DResources();
			__declspec(dllexport) void DiscardDirect2DResources();
			__declspec(dllexport) D2D1_POINT_2F BodyToScreen(const CameraSpacePoint& bodyPoint, int width, int height);
			__declspec(dllexport) HRESULT InitializeDefaultSensor();
			__declspec(dllexport) void BreathUpdate();
			__declspec(dllexport) void BreathUpdate2(float data);
			__declspec(dllexport) void Calibrate();
			__declspec(dllexport) bool GetBreathingIn();
			__declspec(dllexport) int Test();
		};
	}
}