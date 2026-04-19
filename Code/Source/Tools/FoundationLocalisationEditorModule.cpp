
#include <FoundationLocalisation/FoundationLocalisationTypeIds.h>
#include <FoundationLocalisationModuleInterface.h>
#include "FoundationLocalisationEditorSystemComponent.h"

void InitFoundationLocalisationResources()
{
    // We must register our Qt resources (.qrc file) since this is being loaded from a separate module (gem)
    Q_INIT_RESOURCE(FoundationLocalisation);
}

namespace FoundationLocalisation
{
    class FoundationLocalisationEditorModule
        : public FoundationLocalisationModuleInterface
    {
    public:
        AZ_RTTI(FoundationLocalisationEditorModule, FoundationLocalisationEditorModuleTypeId, FoundationLocalisationModuleInterface);
        AZ_CLASS_ALLOCATOR(FoundationLocalisationEditorModule, AZ::SystemAllocator);

        FoundationLocalisationEditorModule()
        {
            InitFoundationLocalisationResources();

            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                FoundationLocalisationEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<FoundationLocalisationEditorSystemComponent>(),
            };
        }
    };
}// namespace FoundationLocalisation

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), FoundationLocalisation::FoundationLocalisationEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_FoundationLocalisation_Editor, FoundationLocalisation::FoundationLocalisationEditorModule)
#endif
