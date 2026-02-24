// Author: Antonio Sidenko (Tonetfal), February 2026

#pragma once

#include "UObject/FieldPath.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class FDetailWidgetRow;
class IDetailChildrenBuilder;
class SWidget;

class FGTM_GameplayTagBlueprintPropertyMappingDetails
	: public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//~IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
		FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	//~End of IPropertyTypeCustomization Interface

protected:
	void OnChangeProperty(TFieldPath<FProperty> ItemSelected, ESelectInfo::Type SelectInfo);

	FGuid GetPropertyGuid(TFieldPath<FProperty> Property) const;
	FString GetPropertyName(TFieldPath<FProperty> Property) const;

	TSharedRef<SWidget> GeneratePropertyWidget(TFieldPath<FProperty> Property);

	FText GetSelectedValueText() const;

protected:
	TSharedPtr<IPropertyHandle> NamePropertyHandle;
	TSharedPtr<IPropertyHandle> GuidPropertyHandle;

	TArray<TFieldPath<FProperty>> PropertyOptions;

	TFieldPath<FProperty> SelectedProperty;
};
