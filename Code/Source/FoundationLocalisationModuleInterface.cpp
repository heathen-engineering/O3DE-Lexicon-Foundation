
#include "FoundationLocalisationModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <FoundationLocalisation/FoundationLocalisationTypeIds.h>

#include <Clients/FoundationLocalisationSystemComponent.h>

namespace FoundationLocalisation
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(FoundationLocalisationModuleInterface,
        "FoundationLocalisationModuleInterface", FoundationLocalisationModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(FoundationLocalisationModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(FoundationLocalisationModuleInterface, AZ::SystemAllocator);

    FoundationLocalisationModuleInterface::FoundationLocalisationModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            FoundationLocalisationSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList FoundationLocalisationModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<FoundationLocalisationSystemComponent>(),
        };
    }
} // namespace FoundationLocalisation
