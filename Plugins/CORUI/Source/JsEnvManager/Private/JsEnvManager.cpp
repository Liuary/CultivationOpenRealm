// Copyright Epic Games, Inc. All Rights Reserved.

#include "JsEnvManager.h"

#include "ISettingsModule.h"
#include "JsEnvManagerSetting.h"

#define LOCTEXT_NAMESPACE "FJsEnvManagerModule"

const FString SettingsSection = "JsEnvManager";

void FJsEnvManagerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// 注册设置
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", *SettingsSection,
										 FText::FromString("JsEnv Manager Settings"),
										 FText::FromString("Open JsEnv Manager Settings"),
										 GetMutableDefault<UJsEnvManagerSetting>());
	}
}

void FJsEnvManagerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// 取消注册设置
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", *SettingsSection);
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FJsEnvManagerModule, JsEnvManager)