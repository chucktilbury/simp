set(CASE_NAME simp_rejects_missing_module_lists_searched_paths)
set(CASE_FIXTURE negative_import_unregistered.simp)
set(CASE_MODULE_ROOT_SOURCE default)
set(CASE_MODULE_ROOT_MISSING [==[ON]==])
set(CASE_EXPECTED_DIAGNOSTIC [==[module 'missing_module' was not found in any package module root; expected <module-root>/missing_module/<version>/simp-package.toml
searched module roots:
  <PROJECT_DIR>/modules \[not found\] \(project <project-root>/modules\)
  <WORK_DIR>/xdg/simp/modules \[not found\] \(XDG_CONFIG_HOME/simp/modules\)
  [^
]+/share/simp/modules \(executable-relative\)]==])
