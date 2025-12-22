//
// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

//! ProfCollect Binder client interface.

mod config;
mod report;
mod scheduler;
mod service;
mod trace_provider;

use anyhow::{Context, Result};
use nix::sys::sysinfo;
use profcollectd_aidl_interface::aidl::com::android::server::profcollect::IProfCollectd::{
    self, BnProfCollectd,
};
use profcollectd_aidl_interface::aidl::com::android::server::profcollect::IProviderStatusCallback::{IProviderStatusCallback, BnProviderStatusCallback};
use profcollectd_aidl_interface::binder::{self, BinderFeatures};
use service::{err_to_binder_status, ProfcollectdBinderService};
use std::time::Duration;

const PROFCOLLECTD_SERVICE_NAME: &str = "profcollectd";

struct ProviderStatusCallback;

impl binder::Interface for ProviderStatusCallback {}

impl IProviderStatusCallback for ProviderStatusCallback {
    fn onProviderReady(&self) -> binder::Result<()> {
        // If we have waited too long for the provider to be ready, then we have passed
        // boot phase, and no need to collect boot profile.
        const TIMEOUT_TO_COLLECT_BOOT_PROFILE: Duration = Duration::from_secs(3);
        let elapsed = sysinfo::sysinfo()
            .context("Failed to get sysinfo")
            .map_err(err_to_binder_status)?
            .uptime();
        if elapsed < TIMEOUT_TO_COLLECT_BOOT_PROFILE {
            trace_system("boot", 0).map_err(err_to_binder_status)?;
        }
        Ok(())
    }
}

/// Initialize profcollectd service.
pub fn init_service() -> Result<()> {
    binder::ProcessState::start_thread_pool();

    let profcollect_binder_service = ProfcollectdBinderService::new()?;
    binder::add_service(
        PROFCOLLECTD_SERVICE_NAME,
        BnProfCollectd::new_binder(profcollect_binder_service, BinderFeatures::default())
            .as_binder(),
    )
    .context("Failed to register service.")?;

    let cb =
        BnProviderStatusCallback::new_binder(ProviderStatusCallback, BinderFeatures::default());
    get_profcollectd_service()?.registerProviderStatusCallback(&cb)?;

    binder::ProcessState::join_thread_pool();
    Ok(())
}

fn get_profcollectd_service() -> Result<binder::Strong<dyn IProfCollectd::IProfCollectd>> {
    binder::wait_for_interface(PROFCOLLECTD_SERVICE_NAME)
        .context("Failed to get profcollectd binder service, is profcollectd running?")
}

/// Immediately schedule a one-off trace.
pub fn trace_system(tag: &str, duration_ms: i32) -> Result<()> {
    get_profcollectd_service()?.trace_system(tag, duration_ms)?;
    Ok(())
}

/// Process traces.
pub fn process() -> Result<()> {
    get_profcollectd_service()?.process()?;
    Ok(())
}

/// Process traces and report profile.
pub fn report() -> Result<String> {
    Ok(get_profcollectd_service()?.report(report::NO_USAGE_SETTING)?)
}

/// Clear all local data.
pub fn reset() -> Result<()> {
    config::clear_data()?;
    Ok(())
}

/// Inits logging for Android
pub fn init_logging() {
    let max_log_level =
        if cfg!(feature = "test") { log::LevelFilter::Info } else { log::LevelFilter::Error };
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("profcollectd")
            .with_max_level(max_log_level)
            .with_log_buffer(android_logger::LogId::System),
    );
}
