#pragma once

#include "model/ModelExtend.h"

namespace model
{

class MorphTargetAnim : public ModelExtend
{
public:
	MorphTargetAnim(int fps, int num_frames, int num_vertices);

	virtual ModelExtendType Type() const override { return EXT_MORPH_TARGET; }

    virtual std::unique_ptr<ModelExtend> Clone() const override;

	int GetFps() const { return m_fps; }

	int GetNumFrames() const { return m_num_frames; }
	int GetNumVertices() const { return m_num_vertices; }

	void SetFrame(int frame) { m_frame = frame; }
	int  GetFrame() const { return m_frame; }

	void  SetBlend(float blend) { m_blend = blend; }
	float GetBlend() const { return m_blend; }

private:
	int m_fps = 30;

	int m_num_frames = 0;
	int m_num_vertices = 0;

	int m_frame = 0;
	float m_blend = 0;

}; // MorphTargetAnim

}