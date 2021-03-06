#include "ext.hpp"
#include "mmd-physics/motion.hpp"
#include "mmd-physics/armature.hpp"
#include "mmd-physics/body.hpp"
#include <glm/gtc/quaternion.hpp>

using namespace std;
using namespace glm;

namespace mmd {
    namespace physics {

        Motion::~Motion() {}

        class MotionImp : public Motion {
            const pmx::Model *model;
            const vmd::Motion *motion;
            Armature *armature;
            Body *body;

            vector<const vector<vmd::Keyframe*>*> boneRemap;
            vector<const vector<vmd::Face*>*> faceRemap;
            vector<float> morph;

            template <class T>
            pair<int, int> bisect(const T *remap, int frame) {
                if (!remap) return make_pair(-1, -1);
                const T &keys = *remap;
                int size = keys.size();
                int l = 0, r = size;
                if (!r) return make_pair(-1, -1);

                while (r - l > 1) {
                    int m = (l + r) / 2;
                    if (keys[m]->frame <= frame) {
                        l = m;
                    } else {
                        r = m;
                    }
                }

                return r < size ? make_pair(l, r) : make_pair(l, l);
            }

            template <class T, class U, class P>
            void remap(const T &part, U &map, P &remap, const char *tip) {
                int n = part.size();
                remap.resize(n, NULL);
                for (int i = 0; i < n; ++i) {
                    auto x = map.find(part[i].name);
                    if (x != map.end()) {
                        remap[i] = &x->second;
                    }
                    LOG << "map " << tip << " " << i
                        << " keyframes: " << (remap[i] ? remap[i]->size() : 0);
                }
            }

            void remap() {
                remap(model->bones, motion->bones, boneRemap, "bone");
                remap(model->morphs, motion->faces, faceRemap, "face");
            }

            void unmap() {
                boneRemap.clear();
                faceRemap.clear();
            }

            static mat4 toMat4(const vmd::Keyframe &key) {
                mat4 ret = translate(key.position);
                const vec4 &rot = key.rotation;
                quat q(rot.w, rot.x, rot.y, rot.z);
                return ret * mat4_cast(q);
            }

            static mat4 interBone(const vmd::Keyframe &l,
                                  const vmd::Keyframe &r, float s) {

                vec3 pos = l.position;
                const vec4 &rl = l.rotation;
                const vec4 &rr = r.rotation;
                quat q(rl.w, rl.x, rl.y, rl.z);
                quat qr(rr.w, rr.x, rr.y, rr.z);

                pos = mix(pos, r.position, s);
                q = slerp(q, qr, s);

                return translate(pos) * mat4_cast(q);
            }

            static float toFloat(const vmd::Face &face) {
                return face.scalar;
            }

            static float interFace(const vmd::Face &l,
                                   const vmd::Face &r, float s) {

                return mix(l.scalar, r.scalar, s);
            }

            template <class T, class U, class P, class Q>
            Q getFrame(const T *remap, float frame,
                       U trans, P inter, const Q &def) {

                auto res = bisect(remap, frame);
                if (res.first >= 0) {
                    const auto &l = *(*remap)[res.first];
                    if (res.first == res.second) {
                        return trans(l);
                    } else {
                        const auto &r = *(*remap)[res.second];
                        float s = (frame - (float)l.frame) /
                                  ((float)r.frame - (float)l.frame);
                        return inter(l, r, s);
                    }
                } else {
                    return def;
                }
            }

        public:
            MotionImp(bool debug) : model(NULL), motion(NULL),
                          armature(Armature::create()),
                          body(Body::create(debug)) {
                body->bindArmature(armature);
            }

            ~MotionImp() {
                reset();
                delete armature;
                delete body;
            }

            void loadModel(const pmx::Model *m) {
                if (model) resetModel();
                model = m;
                armature->loadModel(m);
                morph.resize(m->morphs.size(), 0);
                if (motion) remap();
            }

            void loadBody() {
                body->loadModel(model);
            }

            void loadMotion(const vmd::Motion *m) {
                LOG << "load motion";
                if (motion) resetMotion();
                motion = m;
                if (model) remap();
            }

            void resetPhysics() {
                body->resetPose();
            }

            void resetPose() {
                if (!model) return;
                armature->resetPose();
                body->resetPose();
                morph.assign(model->morphs.size(), 0);
            }

            void resetMotion() {
                unmap();
                resetPose();
                motion = NULL;
            }

            void resetModel() {
                unmap();
                armature->reset();
                body->reset();
                morph.clear();
                model = NULL;
            }

            void reset() {
                unmap();
                armature->reset();
                body->reset();
                morph.clear();
                model = NULL;
                motion = NULL;
            }

            void updateGlobal(const mat4 &m) {
                body->applyGlobal(m);
            }

            void updateKey(float frame) {
                int n = boneRemap.size();
                for (int i = 0; i < n; ++i) {
                    mat4 trans = getFrame(boneRemap[i], frame,
                                          toMat4, interBone, mat4(1.0f));
                    armature->applyLocal(i, trans);
                }

                armature->solveIK();

                n = faceRemap.size();
                for (int i = 0; i < n; ++i) {
                    morph[i] = getFrame(faceRemap[i], frame,
                                        toFloat, interFace, 0.0f);
                }
            }

            void updatePhysics(float tick) {
                body->update(tick);
            }

            mat4 skin(int index) {
                return armature->skin(index);
            }

            float face(int index) {
                return morph[index];
            }

            const std::vector<glm::vec3> &getDebugLines() {
                return body->getDebugLines();
            }
        };

        Motion *Motion::create(bool debug) {
            return new MotionImp(debug);
        }

    } /* physics */
} /* mmd */
