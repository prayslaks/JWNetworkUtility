// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "IDetailCustomization.h"

/** AI 공급자 설정에 로컬 키 목록과 명시적인 관리 작업을 배치한다. */
class FJWNU_AIProviderKeysCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
