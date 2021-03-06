#ifndef __MMD_PHYSICS_BODY
#define __MMD_PHYSICS_BODY

#include "common.hpp"
#include "armature.hpp"

namespace mmd {
    namespace physics {

        class Body {
        public:
            virtual ~Body();
            virtual void loadModel(const pmx::Model *model) = 0;
            virtual void bindArmature(Armature *armature) = 0;
            virtual void reset(void) = 0;
            virtual void resetPose(void) = 0;
            virtual void applyBone(void) = 0;
            virtual void applyGlobal(const glm::mat4 &m) = 0;
            virtual void stepSimulation(float tick) = 0;
            virtual void updateBone(void) = 0;
            virtual void update(float tick) = 0;

            virtual const std::vector<glm::vec3> &getDebugLines(void) = 0;

            static Body *create(bool debug = false);
        };

    } /* physics */
} /* mmd */

#endif
