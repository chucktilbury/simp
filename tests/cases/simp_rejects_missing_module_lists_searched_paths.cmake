set(CASE_NAME simp_rejects_missing_module_lists_searched_paths)
set(CASE_FIXTURE negative_import_unregistered.simp)
set(CASE_MODULE_ROOT_SOURCE default)
set(CASE_MODULE_ROOT_MISSING [==[ON]==])
set(CASE_EXPECTED_DIAGNOSTIC [==[module 'missing_module' is not registered in any module root or module registry; expected <module-root>/missing_module/<version>/simp-package.toml
searched module roots:
  <PROJECT_DIR>/modules \[not found\] \(default <project-root>/modules\)
  [^
]+/share/simp/modules \(executable-relative\)
searched module registry:
  <WORK_DIR>/simp-modules.tsv \[not found\] \(default ./simp-modules.tsv, deprecated\)]==])
