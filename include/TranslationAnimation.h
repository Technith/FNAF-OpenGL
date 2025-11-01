#pragma once
#include "Object3D.h"
#include "Animation.h"
/**
 * @brief Translates an object at a continuous rate over an interval.
 */
class TranslationAnimation : public Animation {
private:
	/**
	 * @brief How much to increment the position by each second.
	 */
	glm::vec3 m_perSecond;

	/**
	 * @brief Advance the animation by the given time interval.
	 */
	void applyAnimation(float dt) override {
		object().move(m_perSecond * dt);
	}

public:
	/**
	 * @brief Constructs a animation of a constant speed by the given total position
	 * linearly interpolated across the given duration.
	 */
	TranslationAnimation(Object3D& object, float duration, const glm::vec3& moveBy) :
		Animation(object, duration), m_perSecond(moveBy / duration) {}
};

