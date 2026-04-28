// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PmgDesignerExtension.h"
#include "PmgDesignerExtensionFactory.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FPuertsMixinGeneratorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	
	// 获取当前打开的资产
	UObject* GetCurrentAsset();
	// 生成 PuertsMixin 文件
	void GeneratePuertsMixinFile();

private:
	
	void RegisterMenus();

	// 清理所有扩展实例
	void CleanupActiveExtensions();
	
	// 编辑器关闭回调
	void OnEditorClose();
	
	// 委托管理
	FDelegateHandle OnEditorCloseHandle; 	

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	// 界面扩展工厂
	TSharedPtr<FPmgDesignerExtensionFactory> Factory;
	
	// 界面扩展
	TArray<TWeakPtr<FPmgDesignerExtension>> ActiveExtensions;
};
