# Instructions:
# 1. Copy this script into Blender's text editor in the scripting panel (or open it from there).
# 2. Run the script to register the export operator.
# 3. Select the skinned mesh object you want to export. Make sure you select the mesh object, not the armature.
# 4. Go to File > Export > Custom Model Data (.obj, .dmat) to open the export dialog.
# 5. Choose the output directory where you want to save the files.
# 6. Click "Export Model Data (...)" to export the mesh, skeleton, skin weights, bone orientations, root positions, and color texture.


import bpy
import os
import mathutils
import random

# --- Helper Function to find Armature ---


def find_armature(obj):
    """Finds the armature modifier and associated armature object."""
    if obj.type != 'MESH':
        return None, None
    for modifier in obj.modifiers:
        if modifier.type == 'ARMATURE':
            armature_obj = modifier.object
            if armature_obj and armature_obj.type == 'ARMATURE':
                return armature_obj, modifier
    return None, None

# --- Export Functions ---


def export_obj_file(context, obj, filepath):
    """Exports the selected object as OBJ, applying modifiers."""
    print(f"Exporting OBJ to: {filepath}")
    # Ensure only the target object is selected for export
    bpy.ops.object.select_all(action='DESELECT')
    context.view_layer.objects.active = obj
    obj.select_set(True)

    # Export OBJ
    bpy.ops.wm.obj_export(
        filepath=filepath,
        export_selected_objects=True,
        apply_modifiers=False,    
        export_materials=False,   
        export_triangulated_mesh=True,
        global_scale=1.0,
        forward_axis='NEGATIVE_Z',    
        up_axis='Y'
    )
    print("OBJ export complete.")


def export_skeleton_structure(armature_obj, filepath):
    """Exports skeleton joints (rest pose) and bone hierarchy."""
    print(f"Exporting Skeleton Structure to: {filepath}")
    armature_data = armature_obj.data
    if not armature_data:
        print("Error: Armature object has no data.")
        return

    scale = armature_obj.scale

    bones = list(armature_data.bones)
    bone_name_to_index = {bone.name: i for i, bone in enumerate(bones)}

    with open(filepath, 'w') as f:
        f.write("# Skeleton Structure\n")
        for i, bone in enumerate(bones):
            # Apply scale to head and tail positions
            head_pos = bone.head_local
            tail_pos = bone.tail_local
            head_pos_scaled = mathutils.Vector((head_pos.x * scale.x, head_pos.y * scale.y, head_pos.z * scale.z))
            tail_pos_scaled = mathutils.Vector((tail_pos.x * scale.x, tail_pos.y * scale.y, tail_pos.z * scale.z))
            
            f.write(
                f"b {head_pos_scaled.x:.6f} {head_pos_scaled.y:.6f} {head_pos_scaled.z:.6f} {tail_pos_scaled.x:.6f} {tail_pos_scaled.y:.6f} {tail_pos_scaled.z:.6f}\n")

        f.write("\n# Bones parent indices\n")
        inds = list(range(len(bones)))
        random.shuffle(inds)
        for i in inds:
            bone = bones[i]
            # Write the bone index and its parent index
            if bone.parent:
                if bone.parent.name in bone_name_to_index:
                    parent_index = bone_name_to_index[bone.parent.name]
                    f.write(f"p {parent_index} {i}\n")
                else:
                    print(
                        f"Warning: Parent bone '{bone.parent.name}' not found in bone list for bone '{bone.name}'.")

    print("Skeleton structure export complete.")


def export_skin_weights(obj, armature_obj, filepath):
    """Exports skinning weights for each vertex."""
    print(f"Exporting Skin Weights to: {filepath}")
    mesh = obj.data
    if not mesh:
        print("Error: Mesh object has no data.")
        return

    if not obj.vertex_groups:
        print("Warning: Object has no vertex groups (no skinning data).")
        with open(filepath, 'w') as f:
            f.write("# Skinning Weights Exported from Blender\n")
            f.write(
                "# Format: vertex_index bone_name_1 weight_1 bone_name_2 weight_2 ...\n")
            f.write("\n# No vertex groups found.\n")
        print("Skin weights export complete (no weights found).")
        return

    if not armature_obj or not armature_obj.pose or not armature_obj.pose.bones:
        print("Warning: Could not access pose bones for validation.")

    group_index_to_name = {i: vg.name for i,
                           vg in enumerate(obj.vertex_groups)}
    bone_name_to_index = {bone.name: i for i, bone in enumerate(armature_obj.data.bones)}
    with open(filepath, 'w') as f:
        f.write(f"1 {len(mesh.vertices)}\n")

        for i, vertex in enumerate(mesh.vertices):
            best_group_index = -1
            best_bone_weight = 0.0
            # Find the bone with the highest weight.
            for group_element in vertex.groups:
                group_index = group_element.group
                weight = group_element.weight
                if weight > best_bone_weight:
                    best_bone_weight = weight
                    best_group_index = group_index
            # Write the vertex index and the best bone name and weight
            if best_group_index in group_index_to_name \
              and group_index_to_name[best_group_index] in bone_name_to_index\
                and best_bone_weight > 0.6:
                bone_name = group_index_to_name[best_group_index]
                bone_index = bone_name_to_index[bone_name]
                f.write(f"{bone_index}\n")
            else:
                f.write(f"-1\n")

    print("Skin weights export complete.")


def export_bone_orientations(armature_obj, filepath):
    """Exports world-space bone orientations (quaternions) at the current frame."""
    print(f"Exporting Bone Orientations to: {filepath}")
    if not armature_obj or not armature_obj.pose or not armature_obj.pose.bones:
        print("Error: Cannot access pose bones.")
        with open(filepath, 'w') as f:
            f.write(
                "# Bone Orientations (World Space Quaternions) Exported from Blender\n")
            f.write("# Format: bone_name qw qx qy qz\n")
            f.write("\n# No pose bones found.\n")
        print("Bone orientations export complete (no pose bones found).")
        return

    bones = list(armature_obj.data.bones)
    action = armature_obj.animation_data.action
    num_frames = action.frame_range[1] - action.frame_range[0] + 1
    num_bones = len(bones)
    
    all_x = []
    all_y = []
    all_z = []
    all_w = []
    
    with open(filepath, 'w') as f:
        f.write(f"4 {int(num_frames * num_bones)}\n")

        for frame in range(int(action.frame_range[0]), int(action.frame_range[1] + 1)):
            bpy.context.scene.frame_set(int(frame))
            for pose_bone, bone in zip(armature_obj.pose.bones, armature_obj.data.bones):
                mat = bone.matrix_local
                bone_local = pose_bone.matrix_basis
                bone_armature = mat @ bone_local @ mat.inverted()

                local_rotation_quat = bone_armature.to_quaternion()
                
                all_x.append(local_rotation_quat.x)
                all_y.append(local_rotation_quat.y)
                all_z.append(local_rotation_quat.z)
                all_w.append(local_rotation_quat.w)
        
        for x in all_x:
            f.write(f"{x:.6f}\n")
        for y in all_y:
            f.write(f"{y:.6f}\n")
        for z in all_z:
            f.write(f"{z:.6f}\n")
        for w in all_w:
            f.write(f"{w:.6f}\n")

    print("Bone orientations export complete.")


def export_root_positions(armature_obj, filepath):
    """Exports world-space root bone positions for each frame."""
    print(f"Exporting Root Positions to: {filepath}")
    if not armature_obj or not armature_obj.pose or not armature_obj.pose.bones:
        print("Error: Cannot access pose bones.")
        with open(filepath, 'w') as f:
            f.write("# Root Bone Positions Exported from Blender\n")
            f.write("# Format: x y z\n")
            f.write("\n# No pose bones found.\n")
        print("Root positions export complete (no pose bones found).")
        return

    action = armature_obj.animation_data.action
    num_frames = int(action.frame_range[1] - action.frame_range[0] + 1)
    
    all_x = []
    all_y = []
    all_z = []
    
    with open(filepath, 'w') as f:
        f.write(f"3 {num_frames}\n")

        for frame in range(int(action.frame_range[0]), int(action.frame_range[1] + 1)):
            bpy.context.scene.frame_set(int(frame))
            
            root_bone = armature_obj.pose.bones[0]
            world_matrix = armature_obj.matrix_world @ root_bone.matrix
            
            position = world_matrix.translation
            
            all_x.append(position.x) 
            all_y.append(position.z)  # Swap Y and Z
            all_z.append(-position.y)  # Negate Y for forward direction
        
        for x in all_x:
            f.write(f"{x:.6f}\n")
        for y in all_y:
            f.write(f"{y:.6f}\n")
        for z in all_z:
            f.write(f"{z:.6f}\n")

    print("Root positions export complete.")


def export_color_texture(obj, filepath):
    """Exports the color texture of the selected object as a JPG."""
    print(f"Exporting Color Texture to: {filepath}")
    
    if not obj.material_slots:
        print("Warning: Object has no materials.")
        return
        
    material = obj.material_slots[0].material
    if not material:
        print("Warning: First material slot is empty.")
        return
        
    if not material.use_nodes:
        print("Warning: Material does not use nodes.")
        return
        
    color_texture = None
    for node in material.node_tree.nodes:
        if node.type == 'TEX_IMAGE' and node.image:
            color_texture = node.image
            break
            
    if not color_texture:
        print("Warning: No color texture found in material.")
        return
        
    # Save the texture as JPG
    try:
        color_texture.save(filepath=filepath)
        print("Color texture export complete.")
    except Exception as e:
        print(f"Error saving texture: {e}")


# --- Operator Class ---

class EXPORT_OT_custom_model_data(bpy.types.Operator):
    """Exports Mesh (OBJ), Skeleton (TXT), Skin Weights (TXT), and Bone Orientations (TXT)"""
    bl_idname = "export.custom_model_data"
    bl_label = "Export Model Data (OBJ, Skel, Skin, Orient)"
    bl_options = {'REGISTER', 'UNDO'}

    directory: bpy.props.StringProperty(
        name="Output Directory",
        description="Choose the directory where files will be saved",
        subtype='DIR_PATH',
    )

    filter_glob: bpy.props.StringProperty(
        default="",
        options={'HIDDEN'},
        maxlen=255,
    )

    def execute(self, context):
        # --- Input Validation ---
        obj = context.active_object
        if not obj:
            self.report({'ERROR'}, "No active object selected.")
            return {'CANCELLED'}

        if obj.type != 'MESH':
            self.report({'ERROR'}, "Active object is not a Mesh.")
            return {'CANCELLED'}

        armature_obj, armature_modifier = find_armature(obj)
        if not armature_obj:
            self.report(
                {'ERROR'}, "No Armature modifier found on the selected Mesh, or modifier is not linked to an Armature object.")
            return {'CANCELLED'}
        if not armature_obj.data:
            self.report(
                {'ERROR'}, f"Armature object '{armature_obj.name}' has no Armature data.")
            return {'CANCELLED'}

        if not self.directory:
            self.report({'ERROR'}, "Output directory not set.")
            return {'CANCELLED'}

        base_filename = bpy.path.clean_name(obj.name)
        output_dir = bpy.path.abspath(self.directory) 

        obj_filepath = os.path.join(output_dir, f"rest.obj")
        skel_filepath = os.path.join(
            output_dir, f"rest.skel")
        skin_filepath = os.path.join(
            output_dir, f"handles.dmat")
        orient_filepath = os.path.join(
            output_dir, f"pose.dmat")
        rootpos_filepath = os.path.join(
            output_dir, f"rootpos.dmat")
        texture_filepath = os.path.join(
            output_dir, f"texture.jpg")

        # Create directory if it doesn't exist
        os.makedirs(output_dir, exist_ok=True)

        # --- Perform Exports ---
        try:
            # Export Mesh (OBJ)
            export_obj_file(context, obj, obj_filepath)

            # Export Skeleton Structure
            export_skeleton_structure(armature_obj, skel_filepath)

            # Export Skin Weights
            export_skin_weights(obj, armature_obj, skin_filepath)

            # Export Bone Orientations
            export_bone_orientations(armature_obj, orient_filepath)

            # Export Root Positions
            export_root_positions(armature_obj, rootpos_filepath)
            
            # Export Color Texture
            export_color_texture(obj, texture_filepath)

        except Exception as e:
            self.report({'ERROR'}, f"An error occurred during export: {e}")
            import traceback
            traceback.print_exc()  # Print detailed traceback to Blender console
            return {'CANCELLED'}

        self.report(
            {'INFO'}, f"Successfully exported data for '{obj.name}' to {output_dir}")
        return {'FINISHED'}

    def invoke(self, context, event):
        # Automatically set default directory if possible (e.g., blend file dir)
        if not self.directory:
            blend_filepath = context.blend_data.filepath
            if blend_filepath:
                self.directory = os.path.dirname(blend_filepath)
            else:
                # Default to empty if no blend file path is available
                self.directory = ""

        # Open the file browser
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


# --- Registration ---
# Store the classes and menu function for easy access
classes = (
    EXPORT_OT_custom_model_data,
)
_menu_func = None  # Store the menu function reference


def menu_func_export(self, context):
    self.layout.operator(EXPORT_OT_custom_model_data.bl_idname,
                         text="Custom Model Data (.obj, .dmat)")


def register():
    """Registers the operator class and adds it to the menu."""
    global _menu_func
    _menu_func = menu_func_export  # Store the function

    # Register the operator class
    for cls in classes:
        try:
            bpy.utils.register_class(cls)
        except ValueError:  # Catch error if class is already registered
            print(f"Class {cls.__name__} already registered. Skipping.")
            pass  # Allow script to continue

    # Add the menu item
    try:
        # Check if it's already added before appending
        if _menu_func not in bpy.types.TOPBAR_MT_file_export._dyn_ui_initialize():
            bpy.types.TOPBAR_MT_file_export.append(_menu_func)
            print("Appended menu item.")
        else:
            print("Menu item already exists.")
    except Exception as e:
        print(f"Error adding menu item: {e}")

    print("Custom Model Exporter registered.")


def unregister():
    """Unregisters the operator class and removes it from the menu."""
    global _menu_func

    # Remove the menu item safely
    if _menu_func:
        try:
            # Check if it exists before trying to remove
            if _menu_func in bpy.types.TOPBAR_MT_file_export._dyn_ui_initialize():
                bpy.types.TOPBAR_MT_file_export.remove(_menu_func)
                print("Removed menu item.")
            else:
                print(
                    "Menu item not found for removal (might be first run or already removed).")
        except Exception as e:
            print(f"Error removing menu item: {e}")
    else:
        print("Menu function reference not found, cannot remove menu item.")

    # Unregister the operator classes in reverse order
    for cls in reversed(classes):
        if hasattr(bpy.types, cls.__name__):  # Check if class is registered by its name
            try:
                bpy.utils.unregister_class(cls)
                print(f"Unregistered class: {cls.__name__}")
            except RuntimeError as e:
                print(
                    f"Could not unregister class {cls.__name__} (might already be unregistered): {e}")
        else:
            print(
                f"Class {cls.__name__} not found in bpy.types for unregistration.")

    _menu_func = None  # Clear the stored menu function
    print("Custom Model Exporter unregistration attempt finished.")


if __name__ == "__main__":
    print("\n--- Running Registration Script ---")
    # Always attempt to unregister first to clean up previous state
    unregister()
    # Then register the potentially updated code
    register()

    print("\nScript execution finished.")
    print("Find the exporter under 'File > Export > Custom Model Data (...)'.")
    print("Make sure to select your MESH object before exporting.")
    print("The exported OBJ mesh will be in its REST POSE.")

    # Example Usage (after running the script once to register):
    # 1. Select your skinned mesh object.
    # 2. Go to File > Export > Custom Model Data (.obj, .txt)
    # 3. Choose an output directory in the file browser that appears.
    # 4. Click "Export Model Data (...)".
