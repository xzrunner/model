#pragma once

#include "model/Animation.h"

namespace model
{

class MorphTargetAnim : public Animation
{
public:
	MorphTargetAnim(int fps, int num_frames, int num_vertices);

	virtual AnimType Type() const override { return ANIM_MORPH_TARGET; }

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