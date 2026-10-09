set(CASE_NAME cwhip_rejects_missing_module_lists_searched_paths)
set(CASE_FIXTURE negative_import_unregistered.cw)
set(CASE_MODULE_ROOT_SOURCE default)
set(CASE_MODULE_ROOT_MISSING [==[ON]==])
set(CASE_EXPECTED_DIAGNOSTIC [==[module 'missing_module' was not found in any package module root; expected <module-root>/missing_module/<version>/cwhip-package.toml
searched module roots:
  <PROJECT_DIR>/modules \[not found\] \(project <project-root>/modules\)
  <WORK_DIR>/xdg/cwhip/modules \[not found\] \(XDG_CONFIG_HOME/cwhip/modules\)
  [^
]+/share/cwhip/modules \(executable-relative\)]==])
