#pragma once

#include "model/Animation.h"

namespace model
{

class MorphTargetAnim : public Animation
{
public:
	MorphTargetAnim(int fps, int num_frames);

	virtual AnimType Type() const override { return ANIM_MORPH_TARGET; }

	int GetFps() const { return m_fps; }

	int GetNumFrames() const { return m_num_frames; }

	void SetFrame(int frame) { m_frame = frame; }
	int GetFrame() const { return m_frame; }

private:
	int m_fps = 30;

	int m_num_frames = 0;

	int m_frame;

}; // MorphTargetAnim

}