// Copyright Epic Games, Inc. All Rights Reserved.

#include "CORUI.h"

#include "ISettingsModule.h"
#include "CORUI/Config/CORUIManagerSetting.h"

#define LOCTEXT_NAMESPACE "FCORUIModule"

void FCORUIModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// 注册设置
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "UCORUIManager",
										 FText::FromString("COR UIManager Settings"),
										 FText::FromString("Open COR UIManager Settings Settings"),
										 GetMutableDefault<UCORUIManagerSetting>());
	}
}

void FCORUIModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// 注销设置
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "UCORUIManager");
	}
}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCORUIModule, CORUI)