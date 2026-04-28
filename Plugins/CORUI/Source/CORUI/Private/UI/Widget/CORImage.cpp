// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/CORImage.h"

void UCORImage::SetImagePath(FString Path, bool bMatchSize, bool AsyncLoad)
{
	if (Path.IsEmpty())
	{
		return;
	}
	FSoftObjectPath SoftPath(Path);
	if (!SoftPath.IsValid())
	{
		return;
	}
	if (AsyncLoad)
	{
		TSoftObjectPtr<UTexture2D> SoftPathPtr(SoftPath);
		if (!SoftPathPtr.IsNull())
		{
			Super::SetBrushFromSoftTexture(SoftPathPtr, bMatchSize);
		}
	}
	else
	{
		UTexture2D* Texture = Cast<UTexture2D>(SoftPath.TryLoad());
		if (Texture)
		{
			Super::SetBrushFromTexture(Texture, bMatchSize);
		}
	}
}
