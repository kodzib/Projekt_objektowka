#version 120

// Input vertex attributes
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 lightVP; // Light View-Projection matrix

// Output vertex attributes (to fragment shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;
varying vec4 fragPosLightSpace; // Light-space position (for shadows)

void main()
{
    // Send vertex attributes to fragment shader
    vec4 worldPosition = matModel * vec4(vertexPosition, 1.0);
    fragPosition = vec3(worldPosition);
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));

    // Light-space position for shadow mapping
    fragPosLightSpace = lightVP * worldPosition;

    // Final vertex position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
