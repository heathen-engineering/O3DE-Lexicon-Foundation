
#include <FoundationLocalisation/FoundationLocalisationTypeIds.h>
#include <FoundationLocalisationModuleInterface.h>
#include "FoundationLocalisationSystemComponent.h"

namespace FoundationLocalisation
{
    class FoundationLocalisationModule
        : public FoundationLocalisationModuleInterface
    {
    public:
        AZ_RTTI(FoundationLocalisationModule, FoundationLocalisationModuleTypeId, FoundationLocalisationModuleInterface);
        AZ_CLASS_ALLOCATOR(FoundationLocalisationModule, AZ::SystemAllocator);
    };
}// namespace FoundationLocalisation

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), FoundationLocalisation::FoundationLocalisationModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_FoundationLocalisation, FoundationLocalisation::FoundationLocalisationModule)
#endif
