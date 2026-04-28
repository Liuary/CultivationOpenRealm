// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CORWindow.h"
#include "CORFullScreen.generated.h"

/**
 * 
 */
UCLASS()
class CORUI_API UCORFullScreen : public UCORWindow
{
	GENERATED_BODY()

public:
	UCORFullScreen();
	virtual ~UCORFullScreen() override;
};
