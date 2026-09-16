from pathlib import Path

Import("env")

project = Path(env.subst("$PROJECT_DIR")) / "lvgl-ui-project"
lvgl = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV") / "lvgl"
staged = Path(env.subst("$BUILD_DIR")) / "exported-ui-source"
sources = [project / "lvgl_ui_project.c", project / "lvgl_ui_project_gen.c"]
sources += list(project.glob("*.h"))
for directory in ("components", "screens", "fonts", "images"):
    sources += [source for source in (project / directory).rglob("*") if source.suffix in (".c", ".h")]
expected = {source.relative_to(project) for source in sources}
for previous in staged.rglob("*"):
    if previous.is_file() and previous.relative_to(staged) not in expected:
        previous.unlink()
for source in sources:
    target = staged / source.relative_to(project)
    content = source.read_bytes()
    if source.name == "mainscr_gen.c":
        content = content.replace(
            b"lv_obj_t * phony_divers_logo = image_create(row_0, phony_divers_logo);",
            b"lv_obj_t * logo_image = image_create(row_0, phony_divers_logo);",
        ).replace(b"lv_obj_set_name(phony_divers_logo,", b"lv_obj_set_name(logo_image,")
    target.parent.mkdir(parents=True, exist_ok=True)
    if not target.exists() or target.read_bytes() != content:
        target.write_bytes(content)
env.Append(CPPPATH=[str(staged), str(lvgl), str(lvgl / "src")])
library = env.BuildLibrary(
    "$BUILD_DIR/exported-ui",
    str(staged),
    src_filter=[
        "-<*>",
        "+<lvgl_ui_project.c>",
        "+<lvgl_ui_project_gen.c>",
        "+<components/>",
        "+<screens/>",
        "+<fonts/>",
        "+<images/>",
    ],
)
env.Append(LIBS=[library])