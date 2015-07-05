#include "stdafx.h"
#include "BreathingFunctionDLL.h"

namespace BreathingFunctions
{
	HRESULT BreathingClass::InitializeDefaultSensor()
	{
		HRESULT hr;

		hr = GetDefaultKinectSensor(&m_pKinectSensor);
		if (FAILED(hr))
		{
			return hr;
		}

		if (m_pKinectSensor)
		{
			// Initialize the Kinect and get coordinate mapper and the body reader
			IBodyFrameSource* pBodyFrameSource = NULL;

			hr = m_pKinectSensor->Open();

			if (SUCCEEDED(hr))
			{
				hr = m_pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
			}

			if (SUCCEEDED(hr))
			{
				hr = m_pKinectSensor->get_BodyFrameSource(&pBodyFrameSource);
			}

			if (SUCCEEDED(hr))
			{
				hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
			}

			SafeRelease(pBodyFrameSource);
		}

		if (!m_pKinectSensor || FAILED(hr))
		{
			return E_FAIL;
		}

		return hr;
	}

	void BreathingClass::BreathUpdate()
	{
		if (!m_pBodyFrameReader)
		{
			return;
		}

		IBodyFrame* pBodyFrame = NULL;



		HRESULT hr = m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);

		if (SUCCEEDED(hr))
		{
			INT64 nTime = 0;

			hr = pBodyFrame->get_RelativeTime(&nTime);

			IBody* ppBodies[BODY_COUNT] = { 0 };

			if (SUCCEEDED(hr))
			{
				hr = pBodyFrame->GetAndRefreshBodyData(_countof(ppBodies), ppBodies);
			}

			if (SUCCEEDED(hr))
			{
				ProcessBody(nTime, BODY_COUNT, ppBodies);
			}

			for (int i = 0; i < _countof(ppBodies); ++i)
			{
				SafeRelease(ppBodies[i]);
			}
		}

		SafeRelease(pBodyFrame);
	}

	void BreathingClass::ProcessBody(INT64 nTime, int nBodyCount, IBody** ppBodies)
	{
		double testVal = 0;

		if (m_hWnd)
		{
			HRESULT hr = EnsureDirect2DResources();

			if (SUCCEEDED(hr) && m_pRenderTarget && m_pCoordinateMapper)
			{
				m_pRenderTarget->BeginDraw();
				m_pRenderTarget->Clear();

				RECT rct;
				GetClientRect(GetDlgItem(m_hWnd, IDC_VIDEOVIEW), &rct);
				int width = rct.right;
				int height = rct.bottom;



				for (int i = 0; i < nBodyCount; ++i)
				{
					IBody* pBody = ppBodies[i];
					if (pBody)
					{
						BOOLEAN bTracked = false;
						hr = pBody->get_IsTracked(&bTracked);

						if (SUCCEEDED(hr) && bTracked)
						{
							Joint joints[JointType_Count];
							D2D1_POINT_2F jointPoints[JointType_Count];
							HandState leftHandState = HandState_Unknown;
							HandState rightHandState = HandState_Unknown;


							pBody->get_HandLeftState(&leftHandState);
							pBody->get_HandRightState(&rightHandState);



							hr = pBody->GetJoints(_countof(joints), joints);
							if (SUCCEEDED(hr))
							{
								for (int j = 0; j < _countof(joints); ++j)
								{
									jointPoints[j] = BodyToScreen(joints[j].Position, width, height);
								}

								//DrawHand(leftHandState, jointPoints[JointType_ShoulderLeft]);

								float shoulderLeft = jointPoints[JointType_ShoulderLeft].y;
								float shoulderRight = jointPoints[JointType_ShoulderRight].y;
								float shoulderSpine = jointPoints[JointType_SpineShoulder].y;

								// average shoulder height
								testVal = (shoulderLeft + shoulderRight) / 2.0f * 100;

								BreathUpdate2(testVal);
							}
						}
					}
				}

				hr = m_pRenderTarget->EndDraw();

				// Device lost, need to recreate the render target
				// We'll dispose it now and retry drawing
				if (D2DERR_RECREATE_TARGET == hr)
				{
					hr = S_OK;
					DiscardDirect2DResources();
				}
			}

			if (!m_nStartTime)
			{
				m_nStartTime = nTime;
			}


			double fps = 0;

			if (GetBreathingIn())
				fps = 1.0f;
			else
				fps = 0.0f;

			LARGE_INTEGER qpcNow = { 0 };
			if (m_fFreq)
			{
				if (QueryPerformanceCounter(&qpcNow))
				{
					if (m_nLastCounter)
					{
						m_nFramesSinceUpdate++;
						// fps = m_fFreq * m_nFramesSinceUpdate / double(qpcNow.QuadPart - m_nLastCounter);
					}
				}
			}

			WCHAR szStatusMessage[64];
			//StringCchPrintf(szStatusMessage, _countof(szStatusMessage), L" FPS = %0.2f    Time = %I64d", fps, (nTime - m_nStartTime));

		
		}
	}

	HRESULT BreathingClass::EnsureDirect2DResources()
	{
		HRESULT hr = S_OK;

		if (m_pD2DFactory && !m_pRenderTarget)
		{
			RECT rc;
			GetWindowRect(GetDlgItem(m_hWnd, IDC_VIDEOVIEW), &rc);

			int width = rc.right - rc.left;
			int height = rc.bottom - rc.top;
			D2D1_SIZE_U size = D2D1::SizeU(width, height);
			D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
			rtProps.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
			rtProps.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

			// Create a Hwnd render target, in order to render to the window set in initialize
			hr = m_pD2DFactory->CreateHwndRenderTarget(
				rtProps,
				D2D1::HwndRenderTargetProperties(GetDlgItem(m_hWnd, IDC_VIDEOVIEW), size),
				&m_pRenderTarget
				);

			if (FAILED(hr))
			{
				return hr;
			}

			// light green
			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.27f, 0.75f, 0.27f), &m_pBrushJointTracked);

			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow, 1.0f), &m_pBrushJointInferred);
			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green, 1.0f), &m_pBrushBoneTracked);
			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray, 1.0f), &m_pBrushBoneInferred);

			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red, 0.5f), &m_pBrushHandClosed);
			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green, 0.5f), &m_pBrushHandOpen);
			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue, 0.5f), &m_pBrushHandLasso);
		}

		return hr;
	}

	void BreathingClass::DiscardDirect2DResources()
	{
		SafeRelease(m_pRenderTarget);

		SafeRelease(m_pBrushJointTracked);
		SafeRelease(m_pBrushJointInferred);
		SafeRelease(m_pBrushBoneTracked);
		SafeRelease(m_pBrushBoneInferred);

		SafeRelease(m_pBrushHandClosed);
		SafeRelease(m_pBrushHandOpen);
		SafeRelease(m_pBrushHandLasso);
	}

	D2D1_POINT_2F BreathingClass::BodyToScreen(const CameraSpacePoint& bodyPoint, int width, int height)
	{
		// Calculate the body's position on the screen
		DepthSpacePoint depthPoint = { 0 };
		m_pCoordinateMapper->MapCameraPointToDepthSpace(bodyPoint, &depthPoint);

		float screenPointX = static_cast<float>(depthPoint.X * width) / cDepthWidth;
		float screenPointY = static_cast<float>(depthPoint.Y * height) / cDepthHeight;

		return D2D1::Point2F(screenPointX, screenPointY);
	}

	void BreathingClass::BreathUpdate2(float data)
	{
		// update frame count
		breathingVars.frameCount++;

		breathingVars.logY += data;

		// time to calc stored data in logY
		if (breathingVars.frameCount > breathingVars.sampleSize)
		{
			// has calibrated
			if (breathingVars.calibration != 0.0f)
			{
				breathingVars.previousReading = breathingVars.currentReading;
				breathingVars.currentReading = breathingVars.logY / breathingVars.frameCount;
				breathingVars.logY = 0.0f;
				breathingVars.frameCount = 0.0f;
			}
			else
			{
				breathingVars.logY = 0.0f;
				breathingVars.frameCount = 0.0f;
				breathingVars.calibration = breathingVars.logY / breathingVars.frameCount;
			}
		}
	}

	void BreathingClass::Calibrate()
	{
		breathingVars.calibration = 0.0f;
	}

	bool BreathingClass::GetBreathingIn()
	{
		if (breathingVars.previousReading != 0.0f)
		{

			if (std::abs(breathingVars.previousReading - breathingVars.currentReading) > breathingVars.buffer)
			{
				// breathing in
				if (breathingVars.currentReading > breathingVars.previousReading)
					return true;
				else
					return false;
			}
		}

		return false;
	}

	int Test()
	{
		return 7;
	}
}


