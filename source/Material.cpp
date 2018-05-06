#include "model/Material.h"
#include "model/Callback.h"

namespace model
{

MaterialOld::~MaterialOld()
{
	if (texture) {
		Callback::ReleaseImg(texture);
		texture = nullptr;
	}
}

}