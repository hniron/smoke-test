#include "graph/operator_reg.h"
#include "register/op_impl_registry.h"

namespace ge {

REG_OP(AivAicpuNoop)
    .INPUT(config, TensorType({DT_INT64}))
    .OUTPUT(status, TensorType({DT_INT32}))
    .OP_END_FACTORY_REG(AivAicpuNoop)

REG_OP(AivAicpuPollFlags)
    .INPUT(flags, TensorType({DT_INT32}))
    .INPUT(config, TensorType({DT_INT64}))
    .OUTPUT(seen_ns, TensorType({DT_INT64}))
    .OUTPUT(poll_iters, TensorType({DT_INT64}))
    .OUTPUT(status, TensorType({DT_INT32}))
    .OP_END_FACTORY_REG(AivAicpuPollFlags)

REG_OP(AivAicpuStampFlag)
    .INPUT(flags, TensorType({DT_INT32}))
    .INPUT(config, TensorType({DT_INT64}))
    .OUTPUT(seen_ns, TensorType({DT_INT64}))
    .OUTPUT(status, TensorType({DT_INT32}))
    .OP_END_FACTORY_REG(AivAicpuStampFlag)

REG_OP(AivAicpuStampOnly)
    .INPUT(flags, TensorType({DT_INT32}))
    .INPUT(config, TensorType({DT_INT64}))
    .OUTPUT(seen_ns, TensorType({DT_INT64}))
    .OUTPUT(status, TensorType({DT_INT32}))
    .OP_END_FACTORY_REG(AivAicpuStampOnly)

REG_OP(AivAicpuReadBench)
    .INPUT(data, TensorType({DT_INT32}))
    .INPUT(config, TensorType({DT_INT64}))
    .OUTPUT(elapsed_ns, TensorType({DT_INT64}))
    .OUTPUT(checksum, TensorType({DT_INT64}))
    .OUTPUT(status, TensorType({DT_INT32}))
    .OP_END_FACTORY_REG(AivAicpuReadBench)

REG_OP(AivAicpuReadChase)
    .INPUT(next, TensorType({DT_INT32}))
    .INPUT(config, TensorType({DT_INT64}))
    .OUTPUT(elapsed_ns, TensorType({DT_INT64}))
    .OUTPUT(checksum, TensorType({DT_INT64}))
    .OUTPUT(status, TensorType({DT_INT32}))
    .OP_END_FACTORY_REG(AivAicpuReadChase)

} // namespace ge

namespace ops {
namespace {

static ge::graphStatus SetVectorAndStatus(gert::InferShapeContext *context, uint32_t vectorOutCount)
{
    if (context == nullptr) {
        return ge::GRAPH_FAILED;
    }
    const gert::Shape *flagsShape = context->GetInputShape(0);
    if (flagsShape == nullptr || flagsShape->GetDimNum() < 1) {
        return ge::GRAPH_FAILED;
    }
    const int64_t taskCount = flagsShape->GetDim(0);
    for (uint32_t i = 0; i < vectorOutCount; ++i) {
        gert::Shape *shape = context->GetOutputShape(i);
        if (shape == nullptr) {
            return ge::GRAPH_FAILED;
        }
        shape->SetDimNum(1);
        shape->SetDim(0, taskCount);
    }
    gert::Shape *statusShape = context->GetOutputShape(vectorOutCount);
    if (statusShape == nullptr) {
        return ge::GRAPH_FAILED;
    }
    statusShape->SetDimNum(1);
    statusShape->SetDim(0, 1);
    return ge::GRAPH_SUCCESS;
}

static ge::graphStatus SetScalarOutputs(gert::InferShapeContext *context, uint32_t outputCount)
{
    if (context == nullptr) {
        return ge::GRAPH_FAILED;
    }
    for (uint32_t i = 0; i < outputCount; ++i) {
        gert::Shape *shape = context->GetOutputShape(i);
        if (shape == nullptr) {
            return ge::GRAPH_FAILED;
        }
        shape->SetDimNum(1);
        shape->SetDim(0, 1);
    }
    return ge::GRAPH_SUCCESS;
}

static ge::graphStatus InferShapeNoop(gert::InferShapeContext *context)
{
    return SetScalarOutputs(context, 1);
}

static ge::graphStatus InferShapePollFlags(gert::InferShapeContext *context)
{
    return SetVectorAndStatus(context, 2);
}

static ge::graphStatus InferShapeStamp(gert::InferShapeContext *context)
{
    return SetVectorAndStatus(context, 1);
}

static ge::graphStatus InferShapeRead(gert::InferShapeContext *context)
{
    return SetScalarOutputs(context, 3);
}

static ge::graphStatus InferTypeNoop(gert::InferDataTypeContext *context)
{
    if (context == nullptr) {
        return ge::GRAPH_FAILED;
    }
    context->SetOutputDataType(0, ge::DT_INT32);
    return ge::GRAPH_SUCCESS;
}

static ge::graphStatus InferTypePollFlags(gert::InferDataTypeContext *context)
{
    if (context == nullptr) {
        return ge::GRAPH_FAILED;
    }
    context->SetOutputDataType(0, ge::DT_INT64);
    context->SetOutputDataType(1, ge::DT_INT64);
    context->SetOutputDataType(2, ge::DT_INT32);
    return ge::GRAPH_SUCCESS;
}

static ge::graphStatus InferTypeStamp(gert::InferDataTypeContext *context)
{
    if (context == nullptr) {
        return ge::GRAPH_FAILED;
    }
    context->SetOutputDataType(0, ge::DT_INT64);
    context->SetOutputDataType(1, ge::DT_INT32);
    return ge::GRAPH_SUCCESS;
}

static ge::graphStatus InferTypeRead(gert::InferDataTypeContext *context)
{
    if (context == nullptr) {
        return ge::GRAPH_FAILED;
    }
    context->SetOutputDataType(0, ge::DT_INT64);
    context->SetOutputDataType(1, ge::DT_INT64);
    context->SetOutputDataType(2, ge::DT_INT32);
    return ge::GRAPH_SUCCESS;
}

} // namespace

IMPL_OP_INFERSHAPE(AivAicpuNoop)
    .InferShape(InferShapeNoop)
    .InferDataType(InferTypeNoop);

IMPL_OP_INFERSHAPE(AivAicpuPollFlags)
    .InferShape(InferShapePollFlags)
    .InferDataType(InferTypePollFlags);

IMPL_OP_INFERSHAPE(AivAicpuStampFlag)
    .InferShape(InferShapeStamp)
    .InferDataType(InferTypeStamp);

IMPL_OP_INFERSHAPE(AivAicpuStampOnly)
    .InferShape(InferShapeStamp)
    .InferDataType(InferTypeStamp);

IMPL_OP_INFERSHAPE(AivAicpuReadBench)
    .InferShape(InferShapeRead)
    .InferDataType(InferTypeRead);

IMPL_OP_INFERSHAPE(AivAicpuReadChase)
    .InferShape(InferShapeRead)
    .InferDataType(InferTypeRead);

} // namespace ops
