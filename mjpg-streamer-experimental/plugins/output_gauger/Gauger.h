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

class Template {
public:
	Template(void);
	Template(int cX, int cY);
	~Template(void);

	int saveData(const BYTE* img, int imgWidth);
	int saveData(const BYTE* img);
	int saveImage();
	void dump();
	Template& operator=(const Template &t) {
		this->name = t.name;
		this->coordinateX = t.coordinateX;
		this->coordinateY = t.coordinateY;
		this->height = t.height;
		this->width = t.width;
		this->searchRange = t.searchRange;
		this->mmPerPix = t.mmPerPix;
		this->isReference = t.isReference;
		this->isWatched = t.isWatched;
		return *this;
	}

	int getId() { return id; }
	std::string getName() { return name; }
	int getCoordinateX() { return coordinateX; }
	int getCoordinateY() { return coordinateY; }
	int getHeight() { return height; }
	int getWidth() { return width; }
	int getSearchRange() { return searchRange; }
	float getMmPerPix() { return mmPerPix; }
	bool getIsReference() { return isReference; }
	bool getIsWatched() { return isWatched; }
	float getSigma() { return sigma; }
	float getMean() { return mean; }
	float getStepSigma() { return step_sigma; }
	float getStepMean() { return step_mean; }
	BYTE *getData() { return data; }

	void setId(int _id) { id = _id; }
	void setName(std::string _name) { name = _name; }
	void setCoordinateX(int _cx) { coordinateX = _cx; }
	void setCoordinateY(int _cy) { coordinateY = _cy; }
	void setHeight(int _height) { height = _height; }
	void setWidth(int _width) { width = _width; }
	void setSearchRange(int _range) { searchRange = _range; }
	void setMmPerPix(float _mmpix) {mmPerPix = _mmpix; }
	void setIsReference(bool _refer) { isReference = _refer; }
	void setIsWatched(bool _watched) { isWatched = _watched; }
	void setSigma(float _sigma) { sigma = _sigma; }
	void setMean(float _mean) { mean = _mean; }

private:
	int id;
	std::string name;
	int coordinateX;
	int coordinateY;
	int height;
	int width;
	int searchRange;
	float mmPerPix;
	bool isReference;
	bool isWatched;
	float sigma;
	float mean;
	float step_sigma;
	float step_mean;
	BYTE *data;
	
	void init(int cX, int cY);
};

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

	static void MeasureTemplate(const BYTE *imgData, int imgWidth, int x, int y,
	                            int tempHeight, int tempWidth, float *sigma, float *mean);
	static bool MatchTemplate( const BYTE *imgData, int imgWidth, int imgHeight,
	                           Template *temp, float minMatchDegree, float *x, float *y);

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
	static void SinglePixMatch(const BYTE *imgData, int imgWidth, POINT_I pt,
	                           Template *temp, POINT_I &ptOut, float *matchDegree, BYTE step);

public:


private:
	BYTE          *m_pImgData;
	TEMPPARAM      m_tempParam;
	TARGETPARAM    m_targetParam;
	TEMPMATCHPARAM m_matchParm;
	bool           authorized;
};

#endif
