#include "ModelArrange.hpp"


#include <libslic3r/Arrange/SceneBuilder.hpp>

#include <libslic3r/Model.hpp>
#include <libslic3r/Geometry/ConvexHull.hpp>
#include "Arrange/Core/ArrangeItemTraits.hpp"
#include "Arrange/Items/ArrangeItem.hpp"

#include "MTUtils.hpp"

namespace Slic3r {

void duplicate_objects(Model &model, size_t copies_num)
{
    for (ModelObject *o : model.objects) {
        // make a copy of the pointers in order to avoid recursion when appending their copies
        ModelInstancePtrs instances = o->instances;
        for (const ModelInstance *i : instances)
            for (size_t k = 2; k <= copies_num; ++ k)
                o->add_instance(*i);
    }
}

bool arrange_objects(Model &model,
                     const arr2::ArrangeBed &bed,
                     const arr2::ArrangeSettingsView &settings)
{
    arr2::Scene scene{arr2::SceneBuilder{}
                          .set_bed(bed)
                          .set_arrange_settings(settings)
                          .set_model(model)};

    auto task = arr2::ArrangeTaskBase::create(arr2::Tasks::Arrange, scene);
    auto result = task->process();
    return result->apply_on(scene.model());
}

void duplicate_objects(Model &model,
                       size_t copies_num,
                       const arr2::ArrangeBed &bed,
                       const arr2::ArrangeSettingsView &settings)
{
    duplicate_objects(model, copies_num);
    arrange_objects(model, bed, settings);
}

void duplicate(Model &model,
               size_t copies_num,
               const arr2::ArrangeBed &bed,
               const arr2::ArrangeSettingsView &settings)
{
    auto vbh = arr2::VirtualBedHandler::create(bed);
    arr2::DuplicableModel dup_model{&model, std::move(vbh), bounding_box(bed)};

    arr2::Scene scene{arr2::BasicSceneBuilder{}
                          .set_arrangeable_model(&dup_model)
                          .set_arrange_settings(&settings)
                          .set_bed(bed)};

    if (copies_num >= 1)
        copies_num -= 1;

    auto task = arr2::MultiplySelectionTask<arr2::ArrangeItem>::create(scene, copies_num);
    auto result = task->process_native(arr2::DummyCtl{});
    if (result->apply_on(scene.model()))
        dup_model.apply_duplicates();
}

} // namespace Slic3r
