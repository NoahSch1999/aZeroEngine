struct BoundingSphere
{
    float3 Position;
    float Radius;
};

// Why doesn't hlsl support class constructors...
// Maybe look into doing this: https://colinbarrebrisebois.com/2021/11/01/working-around-constructors-in-hlsl-or-lack-thereof/
BoundingSphere CreateBoundingSphere(float3 Position, float Radius) 
{
    BoundingSphere sphere;
    sphere.Position = Position;
    sphere.Radius = Radius;
    return sphere;
}

struct Frustum
{
    float3 Position;
    float4 Rotation;
    float RightSlope;
    float LeftSlope;
    float TopSlope;
    float BottomSlope;
    float Near;
    float Far;
    
    bool Intersects(BoundingSphere sphere)
    {
        // TODO: Implement collision
        return true;
    }
};

Frustum CreateFrustum(float3 position, float4 rotation, float rightSlope, float leftSlope, float topSlope, float bottomSlope, float near, float far)
{
    Frustum frustum;
    frustum.Position = position;
    frustum.Rotation = rotation;
    frustum.RightSlope = rightSlope;
    frustum.LeftSlope = leftSlope;
    frustum.TopSlope = topSlope;
    frustum.BottomSlope = bottomSlope;
    frustum.Near = near;
    frustum.Far = far;
    return frustum;
}

Frustum CreateFrustumFromCameraMatrix(float4x4 viewProjectionMatrix)
{
    // TODO: Calc
    float3 position;
    float4 rotation;
    float rightSlope;
    float leftSlope;
    float topSlope;
    float bottomSlope;
    float near;
    float far;
    return CreateFrustum(position, rotation, rightSlope, leftSlope, topSlope, bottomSlope, near, far);
}