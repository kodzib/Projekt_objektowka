#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 lightVP; // Add this uniform for light-space projection

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out vec4 fragPosLightSpace; // Pass light-space position to fragment shader

void main()
{
    vec4 worldPosition = matModel * vec4(vertexPosition, 1.0);

    // Send vertex attributes to fragment shader
    fragPosition = worldPosition.xyz;
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));

    // Compute light-space position and pass it
    fragPosLightSpace = lightVP * worldPosition;

    // Final position for rasterization
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
