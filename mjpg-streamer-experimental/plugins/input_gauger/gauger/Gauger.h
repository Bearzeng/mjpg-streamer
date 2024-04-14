#pragma once

#ifndef __GAUGER_H__
#define __GAUGER_H__

#ifdef DLL_IMPLEMENT
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

#include <string>
#include "gauger_types.h"

#ifdef WIN32
class DLL_API CGauger {
#else
class CGauger {
#endif

public:
	CGauger(void);
	CGauger(const TARGETPARAM  *targetParam, const TEMPPARAM *tempParam);

	~CGauger(void);
	
	int InitTarget(POINT_I *pt);
	int InitTarget(const BYTE *imgData, POINT_I *pt);
	int IdentifyTarget(const BYTE *imgData, POINT_I *pt);
	bool MeasureTarget(POINT_I &pt, float *tx, float *ty);
	bool MeasureTarget(const BYTE *imgData, POINT_I &pt, float *tx, float *ty);

	float GetTargetTx(void);
	float GetTargetTy(void);

	std::string GetVersion(void);
	bool ReadImg(const char strName[]);

	void SetTargetParam (TARGETPARAM  targetParam);
	TARGETPARAM GetTargetParam ();

	void SetTempParam (TEMPPARAM  tempParam);
	TEMPPARAM GetTempParam ();

private:
	int Authenticate(void);
	float GetModelParaTx(void);
	float GetModelParaTy(void);

	POINT_I GetPix(void);

	bool ZigzagPatternPath(POINT_I pt1, POINT_I pt2, POINT_I &pt, BYTE dPix, BYTE dy,BYTE part1,BYTE part2);
	void GetInitTrans(POINT_F Pt);
	void GetTrans(POINT_F Pt);
	void AffineTrans2D(POINT_I pt, POINT_F &PtOut);
	bool InitAffineTrans2D(POINT_I *pt, POINT_F &PtOut);
	
	void MallocMemory(void);
	void SetImgWidth(int nW);
	void SetImgHeight(int nH);
	bool GetTempPix(POINT_I pt1, POINT_I pt2, POINT_I  &pt, BYTE dx, BYTE dy, BYTE part1, BYTE part2);
	void ScanAlongX(int px1, int px2, int py1, BYTE nx, BYTE ny, BYTE tempType);
	void ScanAlongY(int px1, int py1, int py2, BYTE nx, BYTE ny, BYTE tempType);
	void ScanAlongXX(int px1, int px2, int py1, int nx, int ny, BYTE pixPart1, BYTE pixPart2);
	void ScanAlongYY(int px1, int px2, int py1, int nx, int ny, BYTE pixPart1, BYTE pixPart2);
	void SinglePixMatch(POINT_I pt, BYTE pixPart1, BYTE pixPart2);
	bool FeatureRecognition(BYTE nPix,BYTE beta);
	void InitTemplate(void);

public:


private:
	BYTE          *m_pImgData;
	TEMPPARAM      m_tempParam;
	TARGETPARAM    m_targetParam;
	TEMPMATCHPARAM m_matchParm;
	bool           authorized;
};

#endif
