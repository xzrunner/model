#include "model/MorphTargetAnim.h"

namespace model
{

MorphTargetAnim::MorphTargetAnim(int fps, int num_frames, int num_vertices)
	: m_fps(fps)
	, m_num_frames(num_frames)
	, m_num_vertices(num_vertices)
{
}

std::unique_ptr<ModelExtend> MorphTargetAnim::Clone() const
{
    auto ret = std::make_unique<MorphTargetAnim>(m_fps, m_num_frames, m_num_vertices);
    ret->SetFrame(m_frame);
    ret->SetBlend(m_blend);
    return ret;
}

}